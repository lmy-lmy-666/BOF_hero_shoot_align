"""Launch FULL simulation + hero localization — SELF-CONTAINED.

Everything needed is inside hero_shoot/src/.  No external workspaces required.

Usage:
    cd /home/lmy/hero_shoot
    colcon build --symlink-install
    source install/setup.bash
    ros2 launch hero_localization bringup_sim.launch.py
"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    pkg_localization = get_package_share_directory('hero_localization')
    pkg_sim_adapter = get_package_share_directory('sim_adapter')
    pkg_point_lio = get_package_share_directory('point_lio')
    pkg_bringup = get_package_share_directory('hero_bringup')

    robot_name = 'red_standard_robot1'
    config_file = os.path.join(pkg_localization, 'config', 'sim_params.yaml')
    pcd_file = os.path.join(pkg_bringup, 'pcd', 'Hero.pcd')

    # --- 1. Gazebo simulation + adapters + robot_state_publisher (from sim_adapter) ---
    itl_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_sim_adapter, 'launch', 'sim_test.launch.py')
        ),
    )

    # --- 2. Point-LIO SLAM ---
    pointlio = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_point_lio, 'launch', 'point_lio.launch.py')
        ),
    )

    # --- 3. hero_shoot localization ---
    loam = Node(
        package='hero_localization', executable='loam_adapter',
        namespace=robot_name, name='loam_adapter', output='screen',
        parameters=[config_file],
    )

    reloc = Node(
        package='hero_localization', executable='relocalization',
        namespace=robot_name, name='relocalization', output='screen',
        parameters=[config_file, {'prior_pcd_file': pcd_file}],
    )

    target = Node(
        package='hero_localization', executable='target_computer',
        namespace=robot_name, name='target_computer', output='screen',
        parameters=[config_file],
    )

    ld = LaunchDescription()
    ld.add_action(itl_sim)
    ld.add_action(pointlio)
    ld.add_action(loam)
    ld.add_action(reloc)
    ld.add_action(target)
    return ld
