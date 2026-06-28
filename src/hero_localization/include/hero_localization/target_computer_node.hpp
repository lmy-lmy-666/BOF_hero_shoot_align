#ifndef HERO_LOCALIZATION__TARGET_COMPUTER_NODE_HPP_
#define HERO_LOCALIZATION__TARGET_COMPUTER_NODE_HPP_

#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "hero_interfaces/msg/target_distance.hpp"
#include "visualization_msgs/msg/marker.hpp"

namespace hero_localization
{

/**
 * Periodically look up the muzzle pose from the TF tree (map → muzzle),
 * compute horizontal distance and height difference to the enemy base,
 * and publish them on /hero/target_distance.
 *
 * This is the NAV group's main delivery to the CONTROL group.
 *
 * Parameters:
 *   target_x/y/z    — enemy base coordinates in map frame (m)
 *   muzzle_frame    — TF child frame for gun muzzle (default: "muzzle")
 *   map_frame       — TF parent frame (default: "map")
 *   publish_rate    — output publishing rate in Hz (default: 20.0)
 */
class TargetComputerNode : public rclcpp::Node
{
public:
  explicit TargetComputerNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  void timerCallback();
  void publishLineMarker(double muzzle_x, double muzzle_y, double muzzle_z);

  // --- TF ---
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::unique_ptr<tf2_ros::TransformListener> tf_listener_;

  // --- Publishers ---
  rclcpp::Publisher<hero_interfaces::msg::TargetDistance>::SharedPtr target_pub_;
  rclcpp::Publisher<visualization_msgs::msg::Marker>::SharedPtr marker_pub_;

  // --- Timer ---
  rclcpp::TimerBase::SharedPtr timer_;

  // --- Parameters ---
  double target_x_{0.0};
  double target_y_{0.0};
  double target_z_{0.0};
  std::string muzzle_frame_{"muzzle"};
  std::string map_frame_{"map"};
  double publish_rate_{20.0};
};

}  // namespace hero_localization

#endif  // HERO_LOCALIZATION__TARGET_COMPUTER_NODE_HPP_
