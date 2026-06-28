"""Launch hero localization nodes for real-robot deployment.

Launches:
  1. robot_state_publisher (with hero_robot URDF)
  2. loam_adapter        (Point-LIO → standard TF)
  3. relocalization      (GICP scan-to-map)
  4. target_computer     (muzzle → target_x/target_h)

Usage:
    ros2 launch hero_localization bringup_robot.launch.py \
        prior_pcd_file:=/path/to/Hero.pcd \
        urdf_path:=/path/to/hero_robot.urdf.xacro
"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

import xacro


def generate_launch_description():
    pkg_localization = get_package_share_directory('hero_localization')

    # --- URDF path ---
    # Default: look for hero_description package or fallback to hero_localization
    try:
        pkg_desc = get_package_share_directory('hero_description')
        default_urdf = os.path.join(pkg_desc, 'urdf', 'hero_robot.urdf.xacro')
    except Exception:
        default_urdf = os.path.join(
            pkg_localization, '..', 'hero_description', 'urdf', 'hero_robot.urdf.xacro')

    declare_urdf_path = DeclareLaunchArgument(
        'urdf_path',
        default_value=default_urdf,
        description='Path to hero URDF xacro file'
    )

    # --- Prior map path ---
    default_pcd = os.path.join(
        get_package_share_directory('hero_bringup'), 'pcd', 'Hero.pcd')
    declare_prior_pcd = DeclareLaunchArgument(
        'prior_pcd_file',
        default_value=default_pcd,
        description='Absolute path to prior PCD map file'
    )

    # --- Config file ---
    declare_config = DeclareLaunchArgument(
        'config_file',
        default_value=os.path.join(pkg_localization, 'config', 'robot_params.yaml'),
        description='YAML parameter file for localization nodes'
    )

    # --- robot_state_publisher with URDF ---
    # Xacro mappings for real robot (overridable)
    xacro_mappings = {
        'chassis_height': '0.077',
        'gimbal_yaw_height': '0.026',
        'gimbal_pitch_height': '0.355',
        'muzzle_forward': '0.15',
        'mid360_x': '0.16',
        'mid360_y': '0.0',
        'mid360_z': '0.18',
        'mid360_roll': '0.0',
        'mid360_pitch': '0.5236',   # pi/6 ≈ 30° (real robot)
        'mid360_yaw': '0.0',
    }

    def launch_robot_state_publisher(context):
        urdf_file = context.launch_configurations['urdf_path']
        robot_desc = xacro.process_file(urdf_file, mappings=xacro_mappings).toxml()
        return [Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='hero_state_publisher',
            parameters=[{
                'robot_description': robot_desc,
                'use_sim_time': False,
            }],
        )]

    # --- Localization nodes ---
    load_nodes = Node(
        package='hero_localization',
        executable='loam_adapter',
        name='loam_adapter',
        output='screen',
        parameters=[LaunchConfiguration('config_file')],
    )

    reloc_node = Node(
        package='hero_localization',
        executable='relocalization',
        name='relocalization',
        output='screen',
        parameters=[
            LaunchConfiguration('config_file'),
            {'prior_pcd_file': LaunchConfiguration('prior_pcd_file')},
        ],
    )

    target_node = Node(
        package='hero_localization',
        executable='target_computer',
        name='target_computer',
        output='screen',
        parameters=[LaunchConfiguration('config_file')],
    )

    ld = LaunchDescription()
    ld.add_action(declare_urdf_path)
    ld.add_action(declare_prior_pcd)
    ld.add_action(declare_config)
    ld.add_action(OpaqueFunction(function=launch_robot_state_publisher))
    ld.add_action(load_node)
    ld.add_action(reloc_node)
    ld.add_action(target_node)

    return ld
