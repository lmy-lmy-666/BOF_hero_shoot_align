# 任务计划：英雄吊射导航定位模块

## 目标
实现基于 LiDAR+IMU 的 SLAM 定位系统，交付 `map → muzzle` 的完整 TF 树，以及实时 `target_x`（水平距离）和 `target_h`（高度差），供控制组做弹道解算。

## 核心原则
- **导航组只定位，不碰控制**：不负责云台指令、弹道模型、状态机
- **复用成熟方案**：Point-LIO（里程计）、small_gicp（GICP 重定位）
- **仿真和实车统一架构**：同一套节点，配置区分

## 当前阶段
阶段 6 — 仿真验证完成

## 各阶段

### 阶段 1：需求与架构确认 ✓
- [x] 理解导航组职责和边界
- [x] 确定交付物：TF树 + target_x + target_h
- [x] 确定技术选型（Point-LIO + small_gicp）
- [x] 确认关键参数（目标坐标、场地尺寸、弹丸初速）
- **状态：** complete

### 阶段 2：项目结构搭建 ✓
- [x] 创建 ROS 2 workspace
- [x] 创建 hero_core 包（纯算法，独立编译）
- [x] 创建 hero_localization 包（ROS 2节点）
- [x] 创建 hero_description 包（URDF xacro）
- [x] 创建 hero_bringup 包（顶层 launch 入口）
- [x] 创建 hero_interfaces 包（自定义消息）
- [x] 从 ITL_Hero_Shoot 拷贝仿真依赖包
- **状态：** complete

### 阶段 3：核心算法实现 ✓
- [x] target_computer（muzzle → target_x/target_h/azimuth）
- [x] coordinate_align（yaw_offset 标定）
- [x] 单元测试（gtest，6+5=11 用例全部通过）
- **状态：** complete

### 阶段 4：ROS 2 节点实现 ✓
- [x] loam_adapter_node（Point-LIO → 标准 TF + 点云变换）
- [x] relocalization_node（GICP 重定位 + 先验地图发布）
- [x] target_computer_node（TF 查询 → 距离计算 → Marker 发布）
- [x] URDF xacro（实车/仿真两套参数）
- [x] launch 文件（hero_bringup 一键启动）
- **状态：** complete

### 阶段 5：仿真集成与验证 ✓
- [x] 仿真环境一键启动
- [x] 参数调优（GICP、frame_id、QoS）
- [x] RViz 可视化完善（10 个 display）
- [x] 端到端验证：TF 树完整、target_x/target_h 正确
- **状态：** complete

### 阶段 6：交付 ✓
- [x] README 和使用说明
- [x] 已知问题记录
- [x] 文档更新
- **状态：** complete

## 关键参数

| 参数 | 值 | 来源 |
|------|-----|------|
| 敌方基地坐标 | (23.125, 1.510, 0.840) | sim_params.yaml |
| 弹丸初速 | 16 m/s | 参考项目 |
| 场地尺寸 | 28m × 18m | RM 2026 规则 |
| LiDAR 型号 | MID360 | 仿真模型 |
| ROS 2 版本 | Humble | 系统环境 |

## 项目文件清单

### 导航组原创（5 个包，20 个源文件）

```
hero_core/          纯算法，无 ROS 依赖
├── include/hero_core/target_computer.hpp
├── include/hero_core/coordinate_align.hpp
├── test/test_target_computer.cpp    (6 tests)
├── test/test_coordinate_align.cpp   (5 tests)
├── CMakeLists.txt
└── package.xml

hero_interfaces/    自定义消息
├── msg/TargetDistance.msg
├── CMakeLists.txt
└── package.xml

hero_localization/  3 个 ROS2 节点（交付核心）
├── include/hero_localization/
│   ├── loam_adapter_node.hpp
│   ├── relocalization_node.hpp
│   └── target_computer_node.hpp
├── src/
│   ├── loam_adapter_node.cpp
│   ├── relocalization_node.cpp
│   └── target_computer_node.cpp
├── config/
│   ├── sim_params.yaml
│   └── robot_params.yaml
├── launch/
│   ├── bringup_robot.launch.py
│   └── bringup_sim.launch.py
├── CMakeLists.txt
└── package.xml

hero_description/   URDF
├── urdf/hero_robot.urdf.xacro
├── CMakeLists.txt
└── package.xml

hero_bringup/       顶层入口 + 工具脚本
├── launch/hero_bringup.launch.py
├── scripts/lidar_frame_relay.py
├── pcd/Hero.pcd
├── docs/slam_mapping_guide.md
├── CMakeLists.txt
└── package.xml
```

### 从 ITL_Hero_Shoot 拷贝（仿真依赖，路径为 src/）

```
sim_adapter/              仿真桥接（双yaw→单yaw、Gimbal→JointState）
rmu_gazebo_simulator/     Gazebo 世界 + RViz 配置 + 桥接配置
rmoss_gazebo/             底盘底座驱动
rmoss_gz_resources/       仿真资源
rmoss_interfaces/         自定义消息
point_lio/                Point-LIO 里程计
ign_sim_pointcloud_tool   LiDAR 格式转换（PointXYZ→PointXYZIRT）
pb2025_robot_description  机器人 SDF 模型
sdformat_tools/           SDF 工具
utils/                    工具库
```

## 已做决策

| 决策 | 理由 |
|------|------|
| 导航组不碰云台/弹道/状态机 | 职责边界 |
| 复用 Point-LIO + small_gicp | 成熟方案 |
| target_computer 只发布距离不控制云台 | 控制组自行使用数据 |
| 仿真和实车共用 relocalization/loam_adapter | 同一套节点，配置区分 |
| RViz 固定帧用 map | 全局视角，与参考项目一致 |
| 原创代码与拷贝代码分开 | hero_* 是交付物，其余是仿真依赖 |

## 备注
- 参考项目：/home/lmy/ITL_Hero_Shoot/
- 项目根目录：/home/lmy/BOF_hero_shoot_align/
