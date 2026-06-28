#ifndef HERO_LOCALIZATION__LOAM_ADAPTER_NODE_HPP_
#define HERO_LOCALIZATION__LOAM_ADAPTER_NODE_HPP_

#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "tf2/LinearMath/Transform.h"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_broadcaster.h"
#include "tf2_ros/transform_listener.h"

namespace hero_localization
{

/**
 * Convert Point-LIO odometry output into standard TF and Odometry.
 *
 * Point-LIO publishes odometry in its own "camera_init" frame (gravity-aligned, origin at startup).
 * This node converts it to the standard ROS TF tree:
 *   - Publishes odom → base_footprint TF
 *   - Publishes nav_msgs/Odometry on /odom
 *   - Transforms Point-LIO point clouds from camera_init to odom frame
 *
 * Design:  composition-ready rclcpp::Node (can be used as component).
 */
class LoamAdapterNode : public rclcpp::Node
{
public:
  /** Construct with NodeOptions (enables composition). */
  explicit LoamAdapterNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  // --- Callbacks ---
  void odometryCallback(const nav_msgs::msg::Odometry::ConstSharedPtr msg);
  void pointCloudCallback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr msg);

  // --- Subscriptions ---
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr pcd_sub_;

  // --- Publishers ---
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pcd_pub_;

  // --- TF ---
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::unique_ptr<tf2_ros::TransformListener> tf_listener_;
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

  // --- Timer for fallback TF publishing ---
  rclcpp::TimerBase::SharedPtr tf_timer_;
  void publishTfFallback();

  // --- Parameters ---
  std::string odom_topic_;    // Point-LIO odometry topic (default: aft_mapped_to_init)
  std::string pcd_topic_;     // Point-LIO point cloud topic (default: cloud_registered)
  std::string odom_frame_;    // odom frame name (default: "odom")
  std::string base_frame_;    // base frame name (default: "base_footprint")
  std::string lidar_frame_;   // LiDAR frame name (default: "front_mid360")

  // --- State ---
  bool initialized_{false};
  tf2::Transform T_ext_0_;          // initial base→lidar extrinsic
  tf2::Transform T_init_;           // first Point-LIO pose (gravity-aligned origin)
  tf2::Transform T_pcd_transform_;  // cached camera_init→odom for point cloud
  tf2::Transform last_odom_base_;   // last known odom→base (fallback, starts identity)
};

}  // namespace hero_localization

#endif  // HERO_LOCALIZATION__LOAM_ADAPTER_NODE_HPP_
