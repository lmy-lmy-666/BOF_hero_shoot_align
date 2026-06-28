#ifndef HERO_LOCALIZATION__RELOCALIZATION_NODE_HPP_
#define HERO_LOCALIZATION__RELOCALIZATION_NODE_HPP_

#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "tf2_ros/transform_broadcaster.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <Eigen/Geometry>

namespace hero_localization
{

/**
 * Relocalize the robot by matching live LiDAR scans against a prior PCD map
 * using GICP (Generalized Iterative Closest Point).
 *
 * Subscribes to registered point clouds (in odom frame) and publishes the
 * map → odom transform.  When combined with the odom → base_footprint TF
 * from LoamAdapter, this completes the full map → chassis chain.
 *
 * Internally uses small_gicp for fast, OpenMP-parallelized GICP registration.
 */
class RelocalizationNode : public rclcpp::Node
{
public:
  explicit RelocalizationNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  void scanCallback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr msg);
  bool loadMap(const std::string & path);
  void publishTransform();

  // --- Subscriptions ---
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr scan_sub_;

  // --- Publishers ---
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr prior_map_pub_;

  // --- Timers ---
  rclcpp::TimerBase::SharedPtr transform_timer_;

  // --- TF ---
  std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

  // --- Parameters ---
  int num_threads_{4};
  int num_neighbors_{20};
  double global_leaf_size_{0.12};
  double registered_leaf_size_{0.12};
  double max_dist_sq_{36.0};

  std::string map_frame_{"map"};
  std::string odom_frame_{"odom"};
  std::string prior_pcd_file_;
  std::string input_scan_topic_{"registered_scan"};

  // --- State ---
  bool map_loaded_{false};
  bool has_scan_{false};

  // Prior map cloud (target for GICP)
  pcl::PointCloud<pcl::PointXYZ>::Ptr map_cloud_;

  // Registration result: map→odom (Eigen)
  Eigen::Isometry3d result_T_;           // current estimate
  Eigen::Isometry3d previous_result_T_;  // previous estimate (seed for next frame)
};

}  // namespace hero_localization

#endif  // HERO_LOCALIZATION__RELOCALIZATION_NODE_HPP_
