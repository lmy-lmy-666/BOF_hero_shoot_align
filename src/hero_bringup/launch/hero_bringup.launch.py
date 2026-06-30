"""Hero Shoot — 一键启动（仿真 + 实车 + 建图）

Usage:
    # 仿真定位模式（默认，需要先验地图）
    ros2 launch hero_bringup hero_bringup.launch.py

    # 实车定位模式
    ros2 launch hero_bringup hero_bringup.launch.py mode:=robot prior_pcd_file:=pcd/Hero.pcd

    # 建图模式（Ctrl+C 自动保存 GlobalMap.pcd 到当前目录）
    ros2 launch hero_bringup hero_bringup.launch.py slam:=true
    ros2 launch hero_bringup hero_bringup.launch.py mode:=robot slam:=true
"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
import xacro


def _try_pkg(name):
    try:
        return get_package_share_directory(name)
    except Exception:
        return None


def generate_launch_description():
    pkg_localization = get_package_share_directory('hero_localization')
    pkg_sim_adapter  = _try_pkg('sim_adapter')
    pkg_point_lio    = get_package_share_directory('point_lio')
    pkg_bringup      = get_package_share_directory('hero_bringup')

    # =========================================================================
    # Launch arguments
    # =========================================================================
    declare_mode = DeclareLaunchArgument(
        'mode', default_value='sim', description='sim | robot')

    declare_slam = DeclareLaunchArgument(
        'slam', default_value='false', description='建图模式 (true=开启PCD保存)')

    declare_prior_pcd = DeclareLaunchArgument(
        'prior_pcd_file',
        default_value=os.path.join(pkg_bringup, 'pcd', 'Hero.pcd'),
        description='先验 PCD 地图路径')

    declare_rviz = DeclareLaunchArgument(
        'rviz', default_value='True', description='启动 RViz')

    # =========================================================================
    # Main logic
    # =========================================================================
    def launch_setup(context):
        mode = LaunchConfiguration('mode').perform(context)
        slam = LaunchConfiguration('slam').perform(context)
        prior_pcd = LaunchConfiguration('prior_pcd_file').perform(context)
        use_rviz = LaunchConfiguration('rviz').perform(context)
        is_slam = slam.lower() == 'true'

        config_file = os.path.join(
            pkg_localization, 'config',
            'sim_params.yaml' if mode == 'sim' else 'robot_params.yaml')

        actions = []

        if mode == 'sim':
            # ===== SIMULATION =====
            if pkg_sim_adapter is None:
                raise RuntimeError('sim_adapter 包未安装，仿真模式不可用')

            remap_tf = [
                ('/tf', '/red_standard_robot1/tf'),
                ('/tf_static', '/red_standard_robot1/tf_static'),
            ]

            # Gazebo
            actions.append(IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(pkg_sim_adapter, 'launch', 'sim_test.launch.py'))))

            # Point-LIO
            actions.append(Node(
                package='point_lio', executable='pointlio_mapping',
                name='point_lio', output='screen',
                parameters=[os.path.join(pkg_point_lio, 'config', 'mid360.yaml'),
                            {'use_sim_time': True,
                             'pcd_save.pcd_save_en': is_slam,
                             'publish.tf_send_en': is_slam}],
                remappings=remap_tf))

            if is_slam:
                # 建图模式：只跑 Point-LIO，不启定位节点
                if use_rviz.lower() == 'true':
                    actions.append(Node(
                        package='rviz2', executable='rviz2', name='rviz2',
                        arguments=['-d', os.path.join(
                            pkg_bringup, 'rviz', 'visualize_sim_slam.rviz')],
                        remappings=remap_tf,
                        parameters=[{'use_sim_time': True}]))
            else:
                # 定位模式：relay + 定位节点
                actions.append(Node(
                    package='hero_bringup', executable='lidar_frame_relay',
                    name='lidar_frame_relay', output='screen',
                    parameters=[{'use_sim_time': True}]))

                for name in ['loam_adapter', 'relocalization', 'target_computer']:
                    extra = {}
                    if name == 'relocalization':
                        extra = {'prior_pcd_file': prior_pcd}
                    actions.append(Node(
                        package='hero_localization', executable=name,
                        name=name, output='screen',
                        parameters=[config_file, {'use_sim_time': True, **extra}],
                        remappings=remap_tf))

                if use_rviz.lower() == 'true':
                    actions.append(Node(
                        package='rviz2', executable='rviz2', name='rviz2',
                        arguments=['-d', os.path.join(
                            get_package_share_directory('rmu_gazebo_simulator'),
                            'rviz', 'visualize.rviz')],
                        remappings=remap_tf,
                        parameters=[{'use_sim_time': True}]))

        else:
            # ===== REAL ROBOT =====
            sim_time = False

            # robot_state_publisher
            urdf_file = os.path.join(
                get_package_share_directory('hero_description'),
                'urdf', 'hero_robot.urdf.xacro')
            robot_desc = xacro.process_file(urdf_file, mappings={
                'chassis_height':      '0.077',
                'gimbal_yaw_height':   '0.026',
                'gimbal_pitch_height': '0.355',
                'muzzle_forward':      '0.15',
                'mid360_x':     '0.16',
                'mid360_y':     '0.0',
                'mid360_z':     '0.18',
                'mid360_roll':  '0.0',
                'mid360_pitch': '0.5236',
                'mid360_yaw':   '0.0',
            }).toxml()
            actions.append(Node(
                package='robot_state_publisher',
                executable='robot_state_publisher',
                name='hero_state_publisher',
                parameters=[{'robot_description': robot_desc,
                             'use_sim_time': sim_time}]))

            # Point-LIO
            actions.append(Node(
                package='point_lio', executable='pointlio_mapping',
                name='point_lio', output='screen',
                parameters=[os.path.join(pkg_point_lio, 'config', 'mid360.yaml'),
                            {'use_sim_time': sim_time,
                             'pcd_save.pcd_save_en': is_slam,
                             'publish.tf_send_en': True,
                             'common.lid_topic': '/livox/lidar',
                             'common.imu_topic': '/livox/imu',
                             'preprocess.lidar_type': 1,
                             # === 鲁棒性优化（来自 ITL 实车验证） ===
                             'mapping.lidar_meas_cov': 0.001,      # 更信任雷达匹配
                             'mapping.imu_meas_acc_cov': 0.1,       # 不信任加速度计
                             'mapping.imu_meas_omg_cov': 0.1,       # 不信任陀螺仪
                             # gravity_init 修正为 30° pitch 安装角
                             # 9.81*sin30=4.905, 0, -9.81*cos30=-8.496
                             'mapping.gravity_init': [4.905, 0.0, -8.496],
                             }]))

            if not is_slam:
                # 定位模式
                for name in ['loam_adapter', 'relocalization', 'target_computer']:
                    extra = {}
                    if name == 'relocalization':
                        extra = {'prior_pcd_file': prior_pcd}
                    actions.append(Node(
                        package='hero_localization', executable=name,
                        name=name, output='screen',
                        parameters=[config_file, extra]))

            if use_rviz.lower() == 'true':
                rviz_cfg = 'visualize_robot_slam.rviz' if is_slam else 'visualize_robot.rviz'
                actions.append(Node(
                    package='rviz2', executable='rviz2', name='rviz2',
                    arguments=['-d', os.path.join(
                        pkg_bringup, 'rviz', rviz_cfg)]))

        return actions

    ld = LaunchDescription()
    ld.add_action(declare_mode)
    ld.add_action(declare_slam)
    ld.add_action(declare_prior_pcd)
    ld.add_action(declare_rviz)
    ld.add_action(OpaqueFunction(function=launch_setup))
    return ld
