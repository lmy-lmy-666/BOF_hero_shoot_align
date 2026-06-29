# 发现与决策

## 需求
- 导航组负责英雄机器人吊射系统的**定位模块**
- 交付物：`map → muzzle` TF 树 + 实时 `target_x` + `target_h` + `azimuth`
- 不碰：云台控制、弹道模型(GAF)、状态机、发射逻辑

## 研究发现

### ITL_Hero_Shoot 参考项目
- **位置：** /home/lmy/ITL_Hero_Shoot/
- loam_adapter: Point-LIO 输出 → 标准 odom→base_footprint TF
- relocalization: small_gicp GICP 匹配，发布 map→odom + prior_map
- lob_shot_manager: 7 状态机，弹道求解 + 云台控制（导航组不实现）
- 目标坐标: (23.125, 1.510, 0.84)，子弹初速 16 m/s

### RM 2026 规则关键参数
- 场地: 28m × 18m
- 42mm 弹丸初速上限: ≤16.5 m/s
- 英雄初始血量: 300 HP
- 英雄狙击点: 对基地伤害 ×2.5

### 技术栈
- SLAM: Point-LIO（LiDAR-IMU 紧耦合）
- 重定位: small_gicp（OpenMP 加速的 GICP）
- ROS 2: Humble
- 编译: colcon --symlink-install

## 技术决策

| 决策 | 理由 |
|------|------|
| 原创代码与拷贝代码分开 | hero_* 是交付物，其余是仿真依赖 |
| 复用 Point-LIO + small_gicp | 成熟方案，不改源码只配参数 |
| target_computer 只发布距离 | 导航组职责边界 |
| RViz 固定帧用 map | 全局视角，与参考项目一致 |
| relocalization 发布 prior_map | 参考项目做法，transient_local QoS |
| lidar_frame_relay 修正 frame_id | ros_gz_bridge 输出 scoped 名与 TF 树不匹配 |
| 不依赖 rmoss_core | 避免引入整套仿真框架依赖（已移除） |

## 已解决问题

### RViz 点云不显示 (2026-06-28)
- **现象：** Point-LIO 有数据（~4000 feat/scan），但 RViz 中 PointCloud2 无显示
- **根因：** ros_gz_bridge 输出的 frame_id 为 Gazebo scoped 名（含 `red_standard_robot1` 前缀），与 robot_state_publisher 发布的 TF 帧名（`front_mid360`，无前缀）不匹配
- **修复：** 创建 lidar_frame_relay 节点，订阅原始点云，设 frame_id 为 `front_mid360`，重新发布到 `lidar_fixed`
- **涉及文件：**
  - hero_bringup/scripts/lidar_frame_relay.py（新增）
  - hero_bringup/launch/hero_bringup.launch.py（添加 relay 节点）
  - rmu_gazebo_simulator/rviz/visualize.rviz（topic 改为 lidar_fixed）

### RViz 缺少可视化显示 (2026-06-28)
- **现象：** 参考项目有 PriorMap（灰）、RegisteredScan（绿）、Odometry、Path 等，当前项目缺失
- **修复：** 基于参考 nav.rviz 重写 visualize.rviz，添加全部显示，固定帧改为 map
- **涉及文件：** visualize.rviz

### relocalization 不发布 prior_map (2026-06-28)
- **现象：** 参考项目 relocalization 发布 /prior_map（transient_local），当前版本无此功能
- **修复：** 在 relocalization_node 中添加 prior_map_pub_，加载 PCD 后发布一次
- **涉及文件：** relocalization_node.hpp, relocalization_node.cpp

### lidar_frame_relay 关闭时崩溃 (2026-06-28)
- **现象：** Ctrl-C 时 relay 报 exit code 1，RCLError: rcl_shutdown already called
- **根因：** 多个 Python 节点共享 rcl 上下文，先关的调 rcl_shutdown，后关的重复调用
- **修复：** rclpy.shutdown() 外捕获 RuntimeError
- **涉及文件：** lidar_frame_relay.py

### RViz QoS 不匹配 (2026-06-28)
- **现象：** relay 发布 BEST_EFFORT，RViz 订阅 RELIABLE，消息被丢弃
- **修复：** relay 订阅用 BEST_EFFORT（匹配 bridge），发布用 RELIABLE（匹配 RViz）
- **涉及文件：** lidar_frame_relay.py

### 数据验证 (2026-06-28)

| 项目 | 预期值 | 实际值 | 状态 |
|------|--------|--------|------|
| map→muzzle | (2.75, 6.5, 1.12) | (2.751, 6.500, 1.122) | ✅ |
| target_x（起始） | ~21.0m | 20.97m | ✅ |
| target_h（起始） | ~-0.28m | -0.28m | ✅ |
| azimuth（起始） | ~-0.24 rad | -0.24 rad | ✅ |
| 灰色 prior_map | 可见 | 可见 | ✅ |
| 绿色 registered_scan | 可见 | 可见 | ✅ |
| 白色 raw lidar | 可见 | 可见 | ✅ |
| TargetLine Marker | 可见 | 可见 | ✅ |

## 遗留问题

- **Hero.pcd 与 Gazebo 仿真世界几何不完全匹配**：GICP 可能收敛到错误位置。用 init_pose 硬对齐 + drift guard 兜底。长期需在 Gazebo 仿真中重新采集地图 PCD。
- **parameter_bridge 缺 use_sim_time**：上游 spawn_robots.launch.py 未设置，暂不影响功能
- **Ignition 库版本不匹配**：apt 升级 Ignition Gazebo 后 minor 版本变更（如 4.7.0→4.8.1），导致链接失败。已创建 16 个符号链接解决。如果以后 apt 再次升级，可能需重新创建符号链接。

## 2026-06-29 会话：构建修复

### install 目录残缺导致 rviz2 无机器人模型
- **现象：** rviz2 中收不到机器人模型，target_computer 持续报 TF `map→muzzle` 查找失败（Requested time 0.400000）
- **根因：** BOF install 目录缺失 hero_localization、hero_bringup、point_lio、sim_adapter 等关键包（仅 11 个包，应有 19 个），launch 时混用了 BOF 和 hero_shoot 两个 workspace 的包，导致 TF、robot_description 等资源不一致
- **修复：** 完整重编译全部 19 个包（`colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release`），确保 BOF workspace 自包含
- **触发完整重建的原因：** 之前 BOF 构建未指定 `CMAKE_BUILD_TYPE`（默认 Debug），二进制与 hero_shoot 的 Release 构建不一致

### Ignition 库版本不匹配
- **现象：** `rmoss_gz_plugins` 编译失败，链接器报告缺少 `libignition-common4-graphics.so.4.7.0` 等 16 个特定 minor 版本的 .so 文件
- **根因：** 系统 Ignition Gazebo 库被 apt 升级（如 common4: 4.7.0→4.8.1, transport11: 11.4.1→11.4.2），cmake imported targets 引用的旧版本 .so 已不存在
- **修复：** 创建符号链接将旧版本指向当前版本（`sudo ln -sf libignition-<name>.so.<MAJOR> libignition-<name>.so.<OLD>`）

### 绿色点云闪烁（RegisteredScan 一会儿有、一会儿消失）
- **现象：** RViz 中绿色点云间歇性消失又出现，数据频率正常（~15Hz）
- **排查：** `ros2 topic hz /registered_scan` 频率 15Hz 稳定，排除数据中断；min-max 间隔 0.035~0.179s < 0.5s Decay Time，排除超时
- **根因：** `relocalization` 运行 GICP 匹配，结果更新 `map→odom` TF。每次更新可能导致 odom 帧中的点云经 `map→odom` 变换后位置跳动。配合 0.5s 的 Decay Time，视觉上产生消失效果
- **修复：** `visualize.rviz` 中 `RegisteredScan` 的 Decay Time 从 `0.5` → `3`，减少跳变带来的视觉闪烁；重启 launch 生效
- **涉及文件：** `src/rmu_gazebo_simulator/rmu_gazebo_simulator/rviz/visualize.rviz`、`install/rmu_gazebo_simulator/share/rmu_gazebo_simulator/rviz/visualize.rviz`

### 白色原始点云在机器人自旋时偏转 (2026-06-29)
- **现象：** 机器人原地旋转时，RViz 中白色原始点云（`/red_standard_robot1/livox/lidar_fixed`）出现拖影/偏转
- **根因：** 白色点云经过 `lidar_frame_relay.py` 只修改 `frame_id`，不做运动畸变校正。雷达扫描一帧约 0.1s，自旋时帧内朝向变化大，各点采集时刻的位姿不同但被当作同一时刻渲染，产生偏转
- **对比：** 绿色点云（`/registered_scan`）经 Point-LIO 逐点 IMU 传播做运动补偿，自旋时保持稳定
- **结论：** **这不是 bug，是预期行为。** 算法使用的是绿色点云，白色点云仅作可视化参考。如需消除可在 RViz 中去掉白色点云显示
- **涉及文件：** `lidar_frame_relay.py`（仅转发点云）、`laserMapping.cpp`（绿色点云的逐点补偿逻辑）

## 资源
- ITL_Hero_Shoot: /home/lmy/ITL_Hero_Shoot/
- RM 2026 规则手册: /home/lmy/桌面/RoboMaster 2026 机甲大师超级对抗赛比赛规则手册V2.0.0（20260626）.pdf
- Point-LIO: https://github.com/hku-mars/Point-LIO
- small_gicp: https://github.com/koide3/small_gicp

---
*每次发现新问题或做完决策后更新此文件*
