# Hero Shoot — 导航定位模块

## 快速开始

```bash
cd /home/lmy/hero_shoot
source install/setup.bash

# 仿真模式（默认）
ros2 launch hero_bringup hero_bringup.launch.py

# 实车定位（需先验地图，另开终端先启动雷达驱动）
ros2 launch livox_ros_driver2 msg_MID360_launch.py frame_id:=front_mid360 publish_freq:=10.0
ros2 launch hero_bringup hero_bringup.launch.py mode:=robot prior_pcd_file:=/path/to/Hero.pcd

# 实车建图（另开终端先启动雷达驱动）
ros2 launch livox_ros_driver2 msg_MID360_launch.py frame_id:=front_mid360 publish_freq:=10.0
ros2 launch hero_bringup hero_bringup.launch.py mode:=robot slam:=true

# 查看目标距离数据（新终端）
ros2 topic echo /target_computer/target_distance
```

> **注意**：实车模式需**两个终端**——终端1 启动雷达驱动，终端2 启动 hero_bringup。详见 [MID360 雷达连接与验证](MID360_SETUP.md)。

输出字段：`target_x`（水平距离 m）、`target_h`（高度差 m）、`azimuth`（方位角 rad）、`tf_ready`（TF 就绪标记）。

## 编译

```bash
# 全量编译
cd /home/lmy/hero_shoot
colcon build --symlink-install
source install/setup.bash

# 增量编译（只改了一个包时）
colcon build --symlink-install --packages-select <包名>
```

## 仿真 vs 实车

| 项目 | 仿真 (`mode:=sim`) | 实车 (`mode:=robot`) |
|------|-------------------|---------------------|
| Gazebo 仿真 | ✓ | ✗ |
| TF 总线 | `/red_standard_robot1/tf` | `/tf`（全局） |
| robot_state_publisher | sim_adapter 提供 | 自动启动（实车 URDF 参数） |
| Point-LIO LiDAR 话题 | `/velodyne_points`（转换器输出） | `/livox/lidar`（真实驱动） |
| Point-LIO IMU 话题 | `/red_standard_robot1/livox/imu` | `/livox/imu` |
| lidar_frame_relay | ✓（修正 frame_id） | ✗（不需要） |
| use_sim_time | True | False |
| 参数文件 | `sim_params.yaml` | `robot_params.yaml` |

## RViz 显示（实车）

实车模式下 RViz 根据建图/定位自动切换配置：

| 模式 | rviz 配置 | Fixed Frame |
|------|----------|-------------|
| 建图 (`slam:=true`) | `visualize_robot_slam.rviz` | `camera_init`（Point-LIO 建图原点） |
| 定位（默认） | `visualize_robot.rviz` | `map`（重定位 + 先验地图） |

实车 RViz 显示项：

| 显示名 | 类型 | 话题 | 颜色 | 说明 |
|--------|------|------|------|------|
| Grid | 网格 | — | 灰 | 空间参考网格 |
| TF | 坐标系 | — | 彩色 | 全部 TF 帧 |
| RobotModel | 机器人模型 | `/robot_description` | 灰 | URDF 机器人模型 |
| PriorMap | PointCloud2 | `/prior_map` | 灰，半透明 | 先验 PCD 地图 |
| RegisteredScan | PointCloud2 | `/cloud_registered` | **绿**，2px | Point-LIO 实时配准点云 |
| Odometry | 里程计箭头 | `/odom` | 橙 | 里程计方向 |
| Path | 轨迹 | `/path` | 黄 | 运动轨迹 |
| TargetLine | Marker | `/target_computer/target_line` | 绿线 | 目标水平线 |

## RViz 显示（仿真）

启动后 RViz 会自动显示（固定帧 `map`，俯视视角）：

| 显示名 | 类型 | 话题 | 颜色 | 说明 |
|--------|------|------|------|------|
| Grid | 网格 | — | 灰 | 空间参考网格 |
| TF | 坐标系 | — | 彩色 | 全部 TF 帧 |
| RobotModel | 机器人模型 | `/red_standard_robot1/robot_description` | 灰 | URDF 机器人模型 |
| PriorMap | PointCloud2 | `/prior_map` | 灰，半透明 | 先验 PCD 地图（transient_local） |
| RegisteredScan | PointCloud2 | `/registered_scan` | **绿**，2px | 实时配准点云（odom 帧） |
| RawLidar | PointCloud2 | `/red_standard_robot1/livox/lidar_fixed` | 白 | 原始雷达（frame_id 已修正）**仅仿真** |
| Camera | Camera | `/red_standard_robot1/front_industrial_camera/image` | 彩色 | 相机图像 **仅仿真** |
| Odometry | 里程计箭头 | `/odom` | 橙 | 里程计方向 |
| Path | 轨迹 | `/path` | 黄 | 运动轨迹 |
| TargetLine | Marker | `/target_computer/target_line` | 绿线+红球 | 目标水平线 + 敌方基地位置 |
| LobShotViz | MarkerArray | `/lob_shot/visualization` | — | 吊射可视化（预留）**仅仿真** |

> **实车模式** 建图自动加载 `visualize_robot_slam.rviz`（Fixed Frame: `camera_init`），定位加载 `visualize_robot.rviz`（Fixed Frame: `map`）。去掉了仿真专用显示，RobotModel 话题为 `/robot_description`（全局），实时点云话题为 `/cloud_registered`。

## 查看距离

```bash
source install/setup.bash
ros2 topic echo /target_computer/target_distance
```

输出字段：
```
target_x: 20.97    # 水平距离 (m)，控制组弹道解算核心输入
target_h: -0.28    # 高度差 (m)，muzzle 低于 target 为负
azimuth: -0.24     # 地图系方位角 (rad)
tf_ready: true     # TF 链完整时才为 true
```

## 控制组接口

```
话题: /target_computer/target_distance
类型: hero_interfaces/msg/TargetDistance
字段:
  float64 target_x    水平距离 (m)
  float64 target_h    高度差 (m)  
  float64 azimuth     地图系方位角 (rad)
  bool    tf_ready    TF 链是否完整
```

## 仿真中控制机器人

### 键盘遥控

```bash
source /opt/ros/humble/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r cmd_vel:=/red_standard_robot1/cmd_vel
```

| 按键 | 动作 |
|------|------|
| `i` | 前进 |
| `,` | 后退 |
| `Shift+J` | 左横移 |
| `Shift+L` | 右横移 |
| `j` / `l` | 左转 / 右转 |

### 命令行

```bash
ros2 topic pub /red_standard_robot1/cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5}, angular: {z: 0.0}}" -1
```

### 比赛模式供电

```bash
ros2 topic pub /referee_system/red_standard_robot1/enable_power std_msgs/msg/Bool "{data: true}" -1
```

## 查看 TF 树

```bash
# 一次性快照
ros2 run tf2_tools view_frames

# 实时查 muzzle 在 map 坐标系下的位姿
ros2 run tf2_ros tf2_echo map muzzle
```

完整 TF 链：`map → odom → base_footprint → chassis → gimbal_yaw → gimbal_pitch → muzzle`

## 数据流

```
Gazebo LiDAR
    │
    ▼ ros_gz_bridge
/red_standard_robot1/livox/lidar        (frame_id: Gazebo scoped 名)
    │
    ├──► ign_sim_pointcloud_tool ──► /velodyne_points   (PointXYZIRT 格式)
    │                                    │
    │                              point_lio             (里程计 + 点云)
    │                                    │
    │                              loam_adapter          (TF: odom→base_footprint,
    │                                    │               发布 /registered_scan)
    │                                    │
    │                              relocalization        (GICP 匹配，TF: map→odom,
    │                                    │               发布 /prior_map)
    │                                    │
    │                              target_computer       (/target_computer/target_distance,
    │                                                      /target_computer/target_line)
    │
    └──► lidar_frame_relay ──► /red_standard_robot1/livox/lidar_fixed
                               (frame_id 修正为 front_mid360，供 RViz 显示原始点云)
```

## src/ 目录说明

```
src/
├── hero_core/              ← 导航组：纯算法（target_computer、coordinate_align）
├── hero_interfaces/        ← 导航组：自定义消息（TargetDistance）
├── hero_localization/      ← 导航组：3 个 ROS2 节点（交付物）
│   ├── loam_adapter        — Point-LIO → 标准 TF 适配
│   ├── relocalization      — GICP 重定位 + 先验地图发布
│   └── target_computer     — 距离计算 + 可视化 Marker
├── hero_description/       ← 导航组：简化 URDF（TF 发布用）
├── hero_bringup/           ← 导航组：一键启动入口 + lidar_frame_relay
│
├── sim_adapter/            ← 拷贝自 ITL（仿真桥接）
├── rmu_gazebo_simulator/   ← 拷贝自 ITL（Gazebo 世界 + RViz 配置）
├── rmoss_*/                ← 拷贝自 ITL（仿真控制器）
├── point_lio/              ← 拷贝自 ITL（Point-LIO SLAM）
├── ign_sim_pointcloud_tool ← 拷贝自 ITL（LiDAR 格式转换）
└── pb2025_robot_description← 拷贝自 ITL（机器人 SDF 模型）
```

## 关键配置文件

| 配置 | 路径 |
|------|------|
| 仿真参数（目标坐标、GICP、适配器） | `src/hero_localization/config/sim_params.yaml` |
| RViz 实车定位配置 | `src/hero_bringup/rviz/visualize_robot.rviz` |
| RViz 实车建图配置 | `src/hero_bringup/rviz/visualize_robot_slam.rviz` |
| RViz 仿真配置 | `src/rmu_gazebo_simulator/rmu_gazebo_simulator/rviz/visualize.rviz` |
| 点云转换器 | `src/rmu_gazebo_simulator/rmu_gazebo_simulator/config/cloud_converter.yaml` |
| 机器人 URDF | `src/hero_description/urdf/hero_robot.urdf.xacro` |
| 先验地图 | `src/hero_bringup/pcd/Hero.pcd` |

## 参数速查

参数可以用命令行直接覆盖，不需要改 YAML 文件。格式：`yaml路径:=值`。

### 每次上场必调

| 参数 | 默认值 | 说明 | 用法示例 |
|------|--------|------|---------|
| `prior_pcd_file` | `pcd/Hero.pcd` | 先验地图路径 | `prior_pcd_file:=/home/lmy/map.pcd` |
| `target_computer.ros__parameters.target_x` | `23.125` | 敌方基地 X (m) | `...target_x:=25.0` |
| `target_computer.ros__parameters.target_y` | `1.510` | 敌方基地 Y (m) | `...target_y:=2.0` |
| `target_computer.ros__parameters.target_z` | `0.84` | 敌方基地 Z (m) | `...target_z:=0.9` |
| `relocalization.ros__parameters.init_pose` | `[0,0,0,0,0,0]` | GICP 初始位姿 `[x,y,z,roll,pitch,yaw]`，设成**机器人出发点** | `...init_pose:=[3.0,7.0,0.8,0,0,0]` |

### 装机/换场地时调

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `point_lio.common.lid_topic` | `/livox/lidar` | 雷达点云话题 |
| `point_lio.common.imu_topic` | `/livox/imu` | 雷达 IMU 话题 |
| `relocalization.ros__parameters.input_scan_topic` | `registered_scan` | 重定位输入点云话题 |
| `target_computer.ros__parameters.publish_rate` | `20.0` | 距离数据发布频率 (Hz) |

### 定位精度调优

| 参数 | 默认值 | 说明 | 建议范围 |
|------|--------|------|---------|
| `relocalization.ros__parameters.global_leaf_size` | `0.12` | 地图降采样 (m) | 0.05~0.2，越小越准越慢 |
| `relocalization.ros__parameters.registered_leaf_size` | `0.12` | 实时扫描降采样 (m) | 0.05~0.2 |
| `relocalization.ros__parameters.max_dist_sq` | `36.0` | GICP 最大匹配距离平方 (m²) | 9~100，越大越远越慢 |
| `relocalization.ros__parameters.num_threads` | `4` | GICP 并行线程数 | 1~CPU 核心数 |
| `relocalization.ros__parameters.num_neighbors` | `20` | 协方差最近邻数 | 10~30 |
| `loam_adapter.ros__parameters.lidar_frame` | `front_mid360` | URDF 雷达连杆名称 | 改 URDF 时同步修改 |

### URDF 结构尺寸（仅实车，装好不改）

| 参数 | 仿真值 | 实车值 | 说明 |
|------|--------|--------|------|
| `chassis_height` | `0.063` | `0.077` | 底盘离地高度 (m) |
| `gimbal_yaw_height` | `0.1376` | `0.026` | chassis → yaw Z 偏移 (m) |
| `gimbal_pitch_height` | `0.172` | `0.355` | yaw → pitch Z 偏移 (m) |
| `muzzle_forward` | `0.15` | `0.15` | pitch → muzzle X 偏移 (m) |
| `mid360_x` | `0.16` | `0.16` | LiDAR 安装 (x, m) |
| `mid360_z` | `0.18` | `0.18` | LiDAR 安装 (z, m) |
| `mid360_pitch` | `0.2618` (15°) | `0.5236` (30°) | LiDAR 俯仰安装角 (rad) |

> URDF 参数在 hero_bringup.launch.py 里硬编码，实车如果尺寸不同需要改 launch 文件。

## 调参方法

**方式一：命令行临时覆盖**（推荐调试用），不改文件，立即生效：

```bash
# 实车模式 + 覆盖目标坐标和初始位姿
ros2 launch hero_bringup hero_bringup.launch.py mode:=robot \
  target_computer.ros__parameters.target_x:=25.0 \
  target_computer.ros__parameters.target_y:=2.0 \
  target_computer.ros__parameters.target_z:=0.9 \
  relocalization.ros__parameters.init_pose:=[3.0,7.0,0.8,0,0,0]

# 仿真模式 + 调 GICP 精度
ros2 launch hero_bringup hero_bringup.launch.py \
  relocalization.ros__parameters.global_leaf_size:=0.08 \
  relocalization.ros__parameters.registered_leaf_size:=0.08 \
  relocalization.ros__parameters.max_dist_sq:=25.0 \
  relocalization.ros__parameters.num_threads:=8

# 仿真建图
ros2 launch hero_bringup hero_bringup.launch.py slam:=true

# 实车建图（话题已在 launch 内配置好，无需额外参数）
ros2 launch hero_bringup hero_bringup.launch.py mode:=robot slam:=true
```

**方式二：直接改 YAML 固化**（试出最佳值后）：

| 文件 | 场景 |
|------|------|
| `src/hero_localization/config/sim_params.yaml` | 仿真 |
| `src/hero_localization/config/robot_params.yaml` | 实车 |

改完直接重新 launch，symlink 安装不需要重编译。

### 仿真专用

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `lidar_frame_relay.target_frame_id` | `front_mid360` | relay 输出的 frame_id |
| `lidar_frame_relay.output_topic` | `/red_standard_robot1/livox/lidar_fixed` | relay 输出话题 |

### Point-LIO 核心参数（一般不动）

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `preprocess.lidar_type` | `2` | 默认值；实车 launch 中覆盖为 `1`（Livox），仿真使用默认 `2`（Velodyne 格式） |
| `preprocess.blind` | `0.5` | 近距离盲区 (m) |
| `mapping.imu_en` | `True` | 是否启用 IMU |
| `mapping.acc_norm` | `9.81` | 重力加速度 (m/s²) |
| `mapping.gravity` | `[0,0,-9.81]` | 重力方向 |
| `mapping.filter_size_surf` | `0.2` | 面元降采样 (m) |
| `mapping.filter_size_map` | `0.2` | 地图降采样 (m) |
| `pcd_save.pcd_save_en` | `False` | 建图时自动开启 |
| `pcd_save.interval` | `-1` | -1=结束时一次保存 |

| 问题 | 原因 | 状态 |
|------|------|------|
| Message Filter dropping（启动时） | TF 树未建完，早期帧丢弃 | 无害，稳定后自动恢复 |
| GICP 可能收敛到错误位置 | 先验地图与仿真世界几何不完全匹配 | 用 init_pose 硬对齐 + drift guard |
| parameter_bridge 缺 use_sim_time | 上游 spawn_robots 未设置 | 暂不影响功能 |
| rviz 无点云、Fixed Frame 报错 | ① lidar IP 不对 ② topic 名 / 坐标系名配错 ③ 新 rviz 文件未软链接 | 2025-06-28 已修复，详见下方修改记录 |

### 2025-06-28 实车调试修改记录

| 文件 | 改动 | 原因 |
|------|------|------|
| `MID360_config.json` | IP `144` → `145` | 实际雷达 IP 为 145 |
| `hero_bringup.launch.py` | `lidar_type` 覆盖为 `1`（Livox） | Point-LIO 需按 Livox 格式解析 CustomMsg |
| `hero_bringup.launch.py` | `publish.tf_send_en: True` | SLAM 模式需发布 camera_init 坐标系 |
| `hero_bringup.launch.py` | SLAM 加载 `visualize_robot_slam.rviz`，定位加载 `visualize_robot.rviz` | 两种模式 Fixed Frame 不同 |
| `visualize_robot.rviz` | 话题 `/registered_scan` → `/cloud_registered`，Fixed Frame → `map` | Point-LIO 实际发布话题名 |
| `visualize_robot_slam.rviz`（新建） | 同上 + Fixed Frame → `camera_init` | SLAM 模式无 map 坐标系 |
| `hero_bringup.launch.py` | 新增鲁棒性参数：`lidar_meas_cov=0.001`, `imu_meas_acc/omg_cov=0.1`, `gravity_init=[4.905,0,-8.496]` | 来自 ITL 实车验证：降低 IMU 权重、修正 30° 安装角重力方向 |
| `install/` 目录 | 手动 `ln -s` visualize_robot_slam.rviz | 新文件不会被 `colcon build` 自动安装 |
