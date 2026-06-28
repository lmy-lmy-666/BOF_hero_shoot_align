#!/usr/bin/env python3
"""Relay Livox point cloud with corrected frame_id for RViz visualization.

The ros_gz_bridge produces a frame_id like 'red_standard_robot1/front_mid360'
(model-scoped), but the TF tree (from robot_state_publisher under namespace
red_standard_robot1) publishes frames as 'front_mid360' (relative).

This relay strips the namespace prefix so RViz can resolve the transform chain:
  chassis -> front_mid360  (from robot_state_publisher URDF)
"""
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy, HistoryPolicy
from sensor_msgs.msg import PointCloud2


class LidarFrameRelay(Node):
    def __init__(self):
        super().__init__('lidar_frame_relay')

        self.declare_parameter('input_topic', '/red_standard_robot1/livox/lidar')
        self.declare_parameter('output_topic', '/red_standard_robot1/livox/lidar_fixed')
        self.declare_parameter('target_frame_id', 'front_mid360')

        input_topic = self.get_parameter('input_topic').value
        output_topic = self.get_parameter('output_topic').value
        self.target_frame_id = self.get_parameter('target_frame_id').value

        # Sub: BEST_EFFORT to match ros_gz_bridge SensorDataQoS
        sub_qos = QoSProfile(
            depth=5,
            reliability=ReliabilityPolicy.BEST_EFFORT,
            durability=DurabilityPolicy.VOLATILE,
            history=HistoryPolicy.KEEP_LAST,
        )
        # Pub: RELIABLE to match RViz PointCloud2 display
        pub_qos = QoSProfile(
            depth=5,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.VOLATILE,
            history=HistoryPolicy.KEEP_LAST,
        )

        self.sub = self.create_subscription(
            PointCloud2, input_topic, self.callback, sub_qos)

        self.pub = self.create_publisher(
            PointCloud2, output_topic, pub_qos)

        self.get_logger().info(
            f'LidarFrameRelay: {input_topic} -> {output_topic} '
            f'(frame_id -> {self.target_frame_id})')

    def callback(self, msg: PointCloud2):
        msg.header.frame_id = self.target_frame_id
        self.pub.publish(msg)


def main():
    rclpy.init()
    node = LidarFrameRelay()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        try:
            rclpy.shutdown()
        except RuntimeError:
            pass  # context already shut down by another node


if __name__ == '__main__':
    main()
