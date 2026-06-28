#include "hero_localization/relocalization_node.hpp"

#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl_conversions/pcl_conversions.h>

#include "small_gicp/pcl/pcl_registration.hpp"
#include "small_gicp/util/downsampling_omp.hpp"

#include "tf2_eigen/tf2_eigen.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

#include "rclcpp/qos.hpp"

namespace hero_localization
{

RelocalizationNode::RelocalizationNode(const rclcpp::NodeOptions & options)
: Node("relocalization", options),
  result_T_(Eigen::Isometry3d::Identity()),
  previous_result_T_(Eigen::Isometry3d::Identity()),
  map_cloud_(new pcl::PointCloud<pcl::PointXYZ>)
{
  // === GICP parameters ===
  this->declare_parameter("num_threads", 4);
  this->declare_parameter("num_neighbors", 20);
  this->declare_parameter("global_leaf_size", 0.12);
  this->declare_parameter("registered_leaf_size", 0.12);
  this->declare_parameter("max_dist_sq", 36.0);

  // === Frame IDs ===
  this->declare_parameter("map_frame", "map");
  this->declare_parameter("odom_frame", "odom");

  // === Files and topics ===
  this->declare_parameter("prior_pcd_file", "");
  this->declare_parameter("input_scan_topic", "registered_scan");
  this->declare_parameter("init_pose", std::vector<double>{0., 0., 0., 0., 0., 0.});

  this->get_parameter("num_threads", num_threads_);
  this->get_parameter("num_neighbors", num_neighbors_);
  this->get_parameter("global_leaf_size", global_leaf_size_);
  this->get_parameter("registered_leaf_size", registered_leaf_size_);
  this->get_parameter("max_dist_sq", max_dist_sq_);
  this->get_parameter("map_frame", map_frame_);
  this->get_parameter("odom_frame", odom_frame_);
  this->get_parameter("prior_pcd_file", prior_pcd_file_);
  this->get_parameter("input_scan_topic", input_scan_topic_);

  // Parse initial pose seed [x, y, z, roll, pitch, yaw]
  std::vector<double> init_pose;
  this->get_parameter("init_pose", init_pose);
  if (init_pose.size() >= 6) {
    result_T_.translation() << init_pose[0], init_pose[1], init_pose[2];
    result_T_.linear() =
      (Eigen::AngleAxisd(init_pose[5], Eigen::Vector3d::UnitZ()) *
       Eigen::AngleAxisd(init_pose[4], Eigen::Vector3d::UnitY()) *
       Eigen::AngleAxisd(init_pose[3], Eigen::Vector3d::UnitX()))
        .toRotationMatrix();
  }
  previous_result_T_ = result_T_;

  // === TF infrastructure ===
  tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

  // === TF publish timer (20Hz) ensures map frame exists even before first scan ===
  transform_timer_ = this->create_wall_timer(
    std::chrono::milliseconds(50),
    std::bind(&RelocalizationNode::publishTransform, this));

  // === Load prior map ===
  if (!prior_pcd_file_.empty()) {
    map_loaded_ = loadMap(prior_pcd_file_);
  }

  // === Subscriber ===
  auto qos = rclcpp::QoS(rclcpp::KeepLast(5));
  qos.reliable();
  scan_sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
    input_scan_topic_, qos,
    std::bind(&RelocalizationNode::scanCallback, this, std::placeholders::_1));

  // === Prior map publisher (transient_local so late-joining RViz gets it) ===
  auto map_qos = rclcpp::QoS(rclcpp::KeepLast(1));
  map_qos.transient_local();
  map_qos.reliable();
  prior_map_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
    "prior_map", map_qos);

  // === Publish prior map once for RViz visualization ===
  if (map_loaded_) {
    sensor_msgs::msg::PointCloud2 map_msg;
    pcl::toROSMsg(*map_cloud_, map_msg);
    map_msg.header.frame_id = map_frame_;
    map_msg.header.stamp = this->now();
    prior_map_pub_->publish(map_msg);

    RCLCPP_INFO(this->get_logger(),
      "Published prior_map on /prior_map (frame=%s, %zu pts, transient_local)",
      map_frame_.c_str(), map_cloud_->size());
  }

  RCLCPP_INFO(this->get_logger(),
    "Relocalization started | map=%s threads=%d neighbors=%d leaf=%.2f/%.2f max_dist=%.1f",
    map_loaded_ ? prior_pcd_file_.c_str() : "NOT LOADED",
    num_threads_, num_neighbors_,
    global_leaf_size_, registered_leaf_size_, max_dist_sq_);
}

bool RelocalizationNode::loadMap(const std::string & path)
{
  if (pcl::io::loadPCDFile<pcl::PointXYZ>(path, *map_cloud_) == -1) {
    RCLCPP_ERROR(this->get_logger(),
      "Failed to load prior map: %s", path.c_str());
    return false;
  }
  RCLCPP_INFO(this->get_logger(),
    "Loaded prior map: %s (%zu points)", path.c_str(), map_cloud_->size());

  // Downsample global map for speed
  if (global_leaf_size_ > 0.0 && !map_cloud_->empty()) {
    pcl::VoxelGrid<pcl::PointXYZ> vg;
    vg.setLeafSize(global_leaf_size_, global_leaf_size_, global_leaf_size_);
    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered(new pcl::PointCloud<pcl::PointXYZ>);
    vg.setInputCloud(map_cloud_);
    vg.filter(*filtered);
    map_cloud_ = filtered;
    RCLCPP_INFO(this->get_logger(),
      "Downsampled map to %zu points (leaf=%.2f)",
      map_cloud_->size(), global_leaf_size_);
  }

  return true;
}

void RelocalizationNode::scanCallback(
  const sensor_msgs::msg::PointCloud2::ConstSharedPtr msg)
{
  if (!map_loaded_) {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
      "[Reloc] Map not loaded, cannot relocalize");
    return;
  }

  // --- 1. Convert ROS PointCloud2 → PCL ---
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan_raw(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::fromROSMsg(*msg, *scan_raw);

  if (scan_raw->empty()) return;

  // --- 2. Downsample source scan ---
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan_filtered(new pcl::PointCloud<pcl::PointXYZ>);
  if (registered_leaf_size_ > 0.0) {
    pcl::VoxelGrid<pcl::PointXYZ> vg;
    vg.setLeafSize(registered_leaf_size_, registered_leaf_size_, registered_leaf_size_);
    vg.setInputCloud(scan_raw);
    vg.filter(*scan_filtered);
  } else {
    scan_filtered = scan_raw;
  }

  // --- 3. GICP registration ---
  // Target = prior map (map frame), Source = live scan (odom frame)
  // Result: map_T_odom
  small_gicp::RegistrationPCL<pcl::PointXYZ, pcl::PointXYZ> reg;
  reg.setNumThreads(num_threads_);
  reg.setCorrespondenceRandomness(num_neighbors_);
  reg.setMaxCorrespondenceDistance(std::sqrt(max_dist_sq_));

  reg.setInputTarget(map_cloud_);
  reg.setInputSource(scan_filtered);


  // align() writes aligned source into output; must NOT be same as source/target
  pcl::PointCloud<pcl::PointXYZ> aligned;
  reg.align(aligned, previous_result_T_.matrix().cast<float>());

  // --- 4. Update result ---
  if (reg.hasConverged()) {
    previous_result_T_ = result_T_;
    result_T_ = reg.getFinalTransformation().cast<double>();
    has_scan_ = true;
  } else {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
      "[Reloc] GICP did not converge (fitness=%.3f)", reg.getFitnessScore());
  }

  // --- 5. Publish map → odom TF ---
  publishTransform();
}

void RelocalizationNode::publishTransform()
{
  geometry_msgs::msg::TransformStamped tf_msg;
  tf_msg.header.stamp = this->now();
  tf_msg.header.frame_id = map_frame_;
  tf_msg.child_frame_id = odom_frame_;

  tf_msg.transform = tf2::eigenToTransform(result_T_).transform;
  tf_broadcaster_->sendTransform(tf_msg);
}

}  // namespace hero_localization

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(hero_localization::RelocalizationNode)
