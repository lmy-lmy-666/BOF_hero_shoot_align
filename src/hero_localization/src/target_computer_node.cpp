#include "hero_localization/target_computer_node.hpp"

#include "hero_core/target_computer.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"

namespace hero_localization
{

TargetComputerNode::TargetComputerNode(const rclcpp::NodeOptions & options)
: Node("target_computer", options)
{
  this->declare_parameter("target_x", 23.125);
  this->declare_parameter("target_y", 1.510);
  this->declare_parameter("target_z", 0.84);
  this->declare_parameter("muzzle_frame", "muzzle");
  this->declare_parameter("map_frame", "map");
  this->declare_parameter("publish_rate", 20.0);

  this->get_parameter("target_x", target_x_);
  this->get_parameter("target_y", target_y_);
  this->get_parameter("target_z", target_z_);
  this->get_parameter("muzzle_frame", muzzle_frame_);
  this->get_parameter("map_frame", map_frame_);
  this->get_parameter("publish_rate", publish_rate_);

  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_unique<tf2_ros::TransformListener>(*tf_buffer_);

  target_pub_ = this->create_publisher<hero_interfaces::msg::TargetDistance>(
    "~/target_distance", 10);

  // RViz visualization: horizontal line hero↔enemy base
  marker_pub_ = this->create_publisher<visualization_msgs::msg::Marker>(
    "~/target_line", 10);

  auto period = std::chrono::duration<double>(1.0 / publish_rate_);
  timer_ = this->create_wall_timer(period,
    std::bind(&TargetComputerNode::timerCallback, this));

  RCLCPP_INFO(this->get_logger(),
    "TargetComputer started | target=(%.3f,%.3f,%.3f) map=%s muzzle=%s rate=%.0fHz",
    target_x_, target_y_, target_z_,
    map_frame_.c_str(), muzzle_frame_.c_str(), publish_rate_);
}

void TargetComputerNode::timerCallback()
{
  hero_interfaces::msg::TargetDistance msg;

  try {
    auto tf_stamped = tf_buffer_->lookupTransform(
      map_frame_, muzzle_frame_,
      tf2::TimePointZero,
      tf2::durationFromSec(0.5));

    double mx = tf_stamped.transform.translation.x;
    double my = tf_stamped.transform.translation.y;
    double mz = tf_stamped.transform.translation.z;

    auto result = hero_core::compute_target(
      mx, my, mz, target_x_, target_y_, target_z_);

    msg.target_x = result.target_x;
    msg.target_h = result.target_h;
    msg.azimuth = result.azimuth;
    msg.tf_ready = true;

    publishLineMarker(mx, my, mz);

  } catch (tf2::TransformException & ex) {
    msg.target_x = 0.0;
    msg.target_h = 0.0;
    msg.azimuth = 0.0;
    msg.tf_ready = false;

    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
      "[TargetComputer] TF %s→%s not ready: %s",
      map_frame_.c_str(), muzzle_frame_.c_str(), ex.what());
  }

  target_pub_->publish(msg);
}

void TargetComputerNode::publishLineMarker(
  double muzzle_x, double muzzle_y, double muzzle_z)
{
  visualization_msgs::msg::Marker line;
  line.header.frame_id = map_frame_;
  line.header.stamp = this->now();
  line.ns = "hero_target";
  line.id = 0;
  line.type = visualization_msgs::msg::Marker::LINE_STRIP;
  line.action = visualization_msgs::msg::Marker::ADD;
  line.scale.x = 0.05;  // line width
  line.color.r = 0.0;
  line.color.g = 1.0;
  line.color.b = 0.0;
  line.color.a = 0.8;

  // Horizontal line: muzzle XY → target XY, at muzzle height
  geometry_msgs::msg::Point p1, p2;
  p1.x = muzzle_x;  p1.y = muzzle_y;  p1.z = muzzle_z;
  p2.x = target_x_; p2.y = target_y_; p2.z = muzzle_z;  // same Z = horizontal
  line.points = {p1, p2};

  // Target point (red sphere)
  visualization_msgs::msg::Marker target_sphere;
  target_sphere.header.frame_id = map_frame_;
  target_sphere.header.stamp = this->now();
  target_sphere.ns = "hero_target";
  target_sphere.id = 1;
  target_sphere.type = visualization_msgs::msg::Marker::SPHERE;
  target_sphere.action = visualization_msgs::msg::Marker::ADD;
  target_sphere.scale.x = target_sphere.scale.y = target_sphere.scale.z = 0.15;
  target_sphere.color.r = 1.0;
  target_sphere.color.g = 0.0;
  target_sphere.color.b = 0.0;
  target_sphere.color.a = 0.8;
  target_sphere.pose.position.x = target_x_;
  target_sphere.pose.position.y = target_y_;
  target_sphere.pose.position.z = target_z_;

  marker_pub_->publish(line);
  marker_pub_->publish(target_sphere);
}

}  // namespace hero_localization

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(hero_localization::TargetComputerNode)
