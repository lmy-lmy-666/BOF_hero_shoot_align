#include "hero_localization/loam_adapter_node.hpp"

#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "pcl_ros/transforms.hpp"

namespace hero_localization
{

LoamAdapterNode::LoamAdapterNode(const rclcpp::NodeOptions & options)
: Node("loam_adapter", options)
{
  // --- Declare configurable parameters ---
  this->declare_parameter<std::string>("odom_topic", "aft_mapped_to_init");
  this->declare_parameter<std::string>("pcd_topic", "cloud_registered");
  this->declare_parameter<std::string>("odom_frame", "odom");
  this->declare_parameter<std::string>("base_frame", "base_footprint");
  this->declare_parameter<std::string>("lidar_frame", "front_mid360");

  this->get_parameter("odom_topic", odom_topic_);
  this->get_parameter("pcd_topic", pcd_topic_);
  this->get_parameter("odom_frame", odom_frame_);
  this->get_parameter("base_frame", base_frame_);
  this->get_parameter("lidar_frame", lidar_frame_);

  // === TF infrastructure ===
  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_unique<tf2_ros::TransformListener>(*tf_buffer_);
  tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

  // === Initialize last known odom→base as identity ===
  last_odom_base_.setIdentity();

  // === Fallback TF timer (20Hz) ensures odom→base_footprint always exists ===
  tf_timer_ = this->create_wall_timer(
    std::chrono::milliseconds(50),
    std::bind(&LoamAdapterNode::publishTfFallback, this));

  // --- Publishers ---
  odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 50);
  pcd_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("registered_scan", 50);

  // --- Subscribers ---
  auto qos = rclcpp::QoS(rclcpp::KeepLast(50));
  qos.reliable();

  odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
    odom_topic_, qos,
    std::bind(&LoamAdapterNode::odometryCallback, this, std::placeholders::_1));

  pcd_sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
    pcd_topic_, qos,
    std::bind(&LoamAdapterNode::pointCloudCallback, this, std::placeholders::_1));

  RCLCPP_INFO(this->get_logger(),
    "LoamAdapter started | odom_topic=%s pcd_topic=%s odom=%s base=%s lidar=%s",
    odom_topic_.c_str(), pcd_topic_.c_str(),
    odom_frame_.c_str(), base_frame_.c_str(), lidar_frame_.c_str());
}

//
//  odometryCallback: convert Point-LIO pose → standard odom→base_footprint TF
//
//  Point-LIO publishes poses in the "camera_init" frame (gravity-aligned, origin
//  at startup).  We extract the relative motion Δ since the first frame and
//  express it in the odom frame, correcting for dynamic extrinsic changes
//  (LiDAR may move relative to base_footprint due to gimbal rotation).
//
//  The transform chain:
//    T(odom→base) = T_ext_0 × T_init⁻¹ × T_pointlio × T_ext_t⁻¹
//      where  T_ext_0 = T(base→lidar) at t=0 (static snapshot)
//             T_init  = T_odom(0) = first Point-LIO pose (gravity-compensated origin)
//             T_pointlio = current Point-LIO pose
//             T_ext_t = T(base→lidar) at current time (may differ from t=0
//                       if LiDAR is mounted on a moving part, e.g. gimbal)
//
void LoamAdapterNode::odometryCallback(const nav_msgs::msg::Odometry::ConstSharedPtr msg)
{
  // === First frame: cache static extrinsic and initial pose ===
  if (!initialized_) {
    try {
      auto tf_stamped = tf_buffer_->lookupTransform(
        base_frame_, lidar_frame_,
        msg->header.stamp,
        rclcpp::Duration::from_seconds(1.0));
      tf2::fromMsg(tf_stamped.transform, T_ext_0_);
    } catch (tf2::TransformException & ex) {
      RCLCPP_WARN(this->get_logger(),
        "[LoamAdapter] Waiting for TF %s→%s: %s",
        base_frame_.c_str(), lidar_frame_.c_str(), ex.what());
      return;
    }

    // Cache Point-LIO's first pose.  All subsequent poses are relative to this.
    tf2::fromMsg(msg->pose.pose, T_init_);

    // Precompute: camera_init → odom = T_ext_0 × T_init⁻¹
    // This rotates+translates Point-LIO's camera_init frame into the odom frame.
    T_pcd_transform_ = T_ext_0_ * T_init_.inverse();

    initialized_ = true;
    RCLCPP_INFO(this->get_logger(),
      "[LoamAdapter] Initialized | extrinsic %s→%s cached",
      base_frame_.c_str(), lidar_frame_.c_str());
  }

  // === Query dynamic extrinsic at current time ===
  //    If LiDAR is fixed on chassis, T_ext_t ≈ T_ext_0.
  //    If LiDAR is on gimbal, T_ext_t changes as gimbal rotates.
  tf2::Transform T_ext_t;
  try {
    auto tf_stamped = tf_buffer_->lookupTransform(
      base_frame_, lidar_frame_,
      msg->header.stamp,
      rclcpp::Duration::from_seconds(0.1));
    tf2::fromMsg(tf_stamped.transform, T_ext_t);
  } catch (tf2::TransformException & ex) {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
      "[LoamAdapter] Dynamic TF %s→%s failed: %s",
      base_frame_.c_str(), lidar_frame_.c_str(), ex.what());
    return;
  }

  // === Extract Point-LIO delta (relative to first frame) ===
  tf2::Transform T_pointlio;
  tf2::fromMsg(msg->pose.pose, T_pointlio);

  // Pure relative motion from t=0 to now (strips gravity alignment)
  tf2::Transform T_delta = T_init_.inverse() * T_pointlio;

  // === Compose odom→base_footprint ===
  tf2::Transform T_odom_base = T_ext_0_ * T_delta * T_ext_t.inverse();

  // === Broadcast TF: odom → base_footprint ===
  last_odom_base_ = T_odom_base;  // save for fallback timer

  geometry_msgs::msg::TransformStamped tf_msg;
  tf_msg.header.stamp = this->now();
  tf_msg.header.frame_id = odom_frame_;
  tf_msg.child_frame_id = base_frame_;
  tf_msg.transform = tf2::toMsg(T_odom_base);
  tf_broadcaster_->sendTransform(tf_msg);

  // === Publish Odometry ===
  nav_msgs::msg::Odometry odom_out;
  odom_out.header.stamp = this->now();
  odom_out.header.frame_id = odom_frame_;
  odom_out.child_frame_id = base_frame_;
  const auto & origin = T_odom_base.getOrigin();
  odom_out.pose.pose.position.x = origin.x();
  odom_out.pose.pose.position.y = origin.y();
  odom_out.pose.pose.position.z = origin.z();
  odom_out.pose.pose.orientation = tf2::toMsg(T_odom_base.getRotation());
  odom_pub_->publish(odom_out);
}

//
//  pointCloudCallback: transform Point-LIO cloud from camera_init → odom frame
//
//  Point-LIO outputs clouds in its camera_init frame.  We transform them to
//  the odom frame so downstream nodes (e.g. relocalization) receive them in a
//  consistent coordinate system.
//
void LoamAdapterNode::pointCloudCallback(
  const sensor_msgs::msg::PointCloud2::ConstSharedPtr msg)
{
  if (!initialized_) return;

  // T_pcd_transform_ = T_ext_0 × T_init⁻¹ transforms camera_init → odom.
  // pcl_ros::transformPointCloud applies the full 4x4 transform to every point.
  sensor_msgs::msg::PointCloud2 out;
  pcl_ros::transformPointCloud(odom_frame_, T_pcd_transform_, *msg, out);
  pcd_pub_->publish(out);
}

//
//  publishTfFallback: timer-driven (20Hz) fallback that publishes the last known
//  odom→base_footprint TF.  This keeps the TF tree connected even when point_lio
//  odometry is temporarily unavailable (e.g. during startup or sensor dropout).
//  Initially publishes identity until the first real odometry arrives.
//
void LoamAdapterNode::publishTfFallback()
{
  geometry_msgs::msg::TransformStamped tf_msg;
  tf_msg.header.stamp = this->now();
  tf_msg.header.frame_id = odom_frame_;
  tf_msg.child_frame_id = base_frame_;
  tf_msg.transform = tf2::toMsg(last_odom_base_);
  tf_broadcaster_->sendTransform(tf_msg);
}

}  // namespace hero_localization

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(hero_localization::LoamAdapterNode)
