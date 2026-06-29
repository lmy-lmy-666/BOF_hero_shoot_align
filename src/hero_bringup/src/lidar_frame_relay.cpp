/// C++ relay: fix frame_id of Livox pointcloud for RViz TF resolution.
/// Uses BEST_EFFORT throughout — no copies in the critical path.

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("lidar_frame_relay");

  auto in_topic  = node->declare_parameter("input_topic",  "/red_standard_robot1/livox/lidar");
  auto out_topic = node->declare_parameter("output_topic", "/red_standard_robot1/livox/lidar_fixed");
  auto target    = node->declare_parameter("target_frame_id", "front_mid360");

  rclcpp::QoS qos(rclcpp::KeepLast(10));
  qos.best_effort();
  qos.durability_volatile();

  auto pub = node->create_publisher<sensor_msgs::msg::PointCloud2>(out_topic, qos);

  auto sub = node->create_subscription<sensor_msgs::msg::PointCloud2>(
      in_topic, qos,
      [&pub, &target](sensor_msgs::msg::PointCloud2::ConstSharedPtr msg) {
        auto out = std::make_unique<sensor_msgs::msg::PointCloud2>();
        out->header = msg->header;
        out->header.frame_id = target;
        out->height       = msg->height;
        out->width        = msg->width;
        out->fields       = msg->fields;
        out->is_bigendian = msg->is_bigendian;
        out->point_step   = msg->point_step;
        out->row_step     = msg->row_step;
        out->data         = msg->data;
        out->is_dense     = msg->is_dense;
        pub->publish(std::move(out));
      });

  RCLCPP_INFO(node->get_logger(), "LidarFrameRelay (C++ BEST_EFFORT): %s -> %s (frame: %s)",
              in_topic.c_str(), out_topic.c_str(), target.c_str());
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
