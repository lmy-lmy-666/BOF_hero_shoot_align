# 进度日志

## 会话：2026-06-27

### 阶段 1：需求与架构确认
- **状态：** complete
- 读取 RM 2026 规则手册（英雄 42mm 关键参数）
- 确认导航组职责：只做定位，交付 TF树 + target_x + target_h
- 确认架构：原创 hero_* 包 + 拷贝仿真依赖

### 阶段 2-4：项目搭建、算法、节点
- **状态：** complete
- 创建 5 个原创包：hero_core、hero_interfaces、hero_localization、hero_description、hero_bringup
- 11 个 gtest 用例全部通过
- 3 个 ROS2 节点编译通过

### 阶段 5：功能审计与修复
- **状态：** complete
- 审计发现 11 个编译/依赖问题，全部修复

## 会话：2026-06-28（仿真调试）

### 第一轮：点云显示修复
- ✅ 诊断 RViz 点云不显示（frame_id 不匹配）
- ✅ 创建 lidar_frame_relay 节点修正 frame_id
- ✅ RViz 添加 TargetLine Marker 显示
- ✅ 修复 relay Python 语法错误（rclcpp→rclpy）

### 第二轮：QoS 修复
- ✅ 诊断 QoS 不匹配（relay BEST_EFFORT vs RViz RELIABLE）
- ✅ 修复：订阅用 BEST_EFFORT（匹配 bridge），发布用 RELIABLE（匹配 RViz）

### 第三轮：参考项目对齐
- ✅ relocalization 添加 /prior_map 发布（transient_local QoS）
- ✅ RViz 配置完全重写，对齐参考 nav.rviz
  - 添加 Grid、PriorMap、Odometry、Path 显示
  - 固定帧改为 map
  - 保留 RobotModel、RawLidar、RegisteredScan、TargetLine
- ✅ RViz 去掉 namespace，改为直接 TF remap（对齐参考）
- ✅ 修复 relay 关闭时崩溃（捕获 RuntimeError）
- ✅ 更新全部文档（README.md、task_plan.md、findings.md、progress.md）

### 最终验证
- ✅ prior_map 发布成功（231062 pts，transient_local）
- ✅ relay 正常退出（不再 crash）
- ✅ 无 QoS 警告
- ✅ Point-LIO 里程计正常（robot 运动后位姿变化）
- ✅ 灰色 prior_map + 绿色 registered_scan + 白色 raw lidar + 目标线 → 全部正常

## 项目当前状态

| 组件 | 状态 |
|------|------|
| hero_core（纯算法，11 tests） | ✅ |
| hero_interfaces（TargetDistance） | ✅ |
| loam_adapter_node | ✅ |
| relocalization_node（含 prior_map 发布） | ✅ |
| target_computer_node（含 Marker 可视化） | ✅ |
| hero_description（URDF xacro） | ✅ |
| hero_bringup（一键启动 + relay） | ✅ |
| lidar_frame_relay（frame_id 修正） | ✅ |
| RViz 配置（10 个 display） | ✅ |
| 文档（README + task_plan + findings + progress） | ✅ |

## 修改过的文件汇总

### 导航组原创文件
- hero_bringup/launch/hero_bringup.launch.py
- hero_bringup/scripts/lidar_frame_relay.py（新增）
- hero_bringup/CMakeLists.txt
- hero_bringup/package.xml
- hero_localization/include/hero_localization/relocalization_node.hpp
- hero_localization/src/relocalization_node.cpp

### 仿真依赖文件
- rmu_gazebo_simulator/rviz/visualize.rviz

### 文档文件
- README.md
- task_plan.md
- findings.md
- progress.md

---
*每次会话结束后更新此文件*

## 会话：2026-06-29（构建修复）

### 问题诊断
- ✅ 诊断 rviz2 不显示机器人模型：BOF install 目录残缺（仅 11/19 个包），缺失 hero_localization、hero_bringup 等
- ✅ 对比 hero_shoot 与 BOF_hero_shoot_align 源码 → 完全一致，排除代码差异
- ✅ 发现 BOF 构建为 Debug 模式（无 CMAKE_BUILD_TYPE），hero_shoot 为 Release

### 构建修复
- ✅ 创建 16 个 Ignition 库旧版本符号链接（系统库升级导致 minor 版本不匹配）
- ✅ 完整重编译全部 19 个包（Release 模式），BOF workspace 现已完全自包含
- ✅ 验证 install 目录完整性：19 个包全部就位

### 文档更新
- ✅ README.md：路径修正、构建说明、已知问题表格、2026-06-29 修改记录
- ✅ task_plan.md：项目路径修正
- ✅ findings.md：新增 Ignition 库问题和构建修复记录
- ✅ progress.md：新增本日会话记录

### 绿色点云闪烁修复
- ✅ 诊断：数据频率 15Hz 正常，排除中断；确认 relocalization GICP 更新 map→odom TF 导致 odom 帧点云位置跳动
- ✅ 修复：`visualize.rviz` 中 RegisteredScan Decay Time `0.5` → `3`，减少跳变闪烁
- ✅ 更新 findings.md 记录该问题

### 当前状态
| 组件 | 状态 |
|------|------|
| 全部 19 个包编译安装 | ✅ |
| BOF workspace 自包含 | ✅ |
| Ignition 库符号链接 | ✅ (16 个) |
| RegisteredScan Decay Time | ✅ (0.5→3) |
| 文档同步 | ✅ |

## 会话：2026-06-29（白点云偏转分析）

### 问题诊断
- ✅ 用户反馈：机器人自旋时白色原始点云偏转
- ✅ 根因分析：`lidar_frame_relay.py` 只改 frame_id，不做运动畸变校正。雷达扫描一帧约 0.1s，自旋时帧内朝向变化大
- ✅ 对比验证：绿色点云经 Point-LIO 逐点 IMU 补偿，自旋时稳定

### 结论
- 白色点云偏转是预期行为，不影响算法（算法用绿色点云）
- 已记录到 findings.md

### 文档更新
- ✅ findings.md：新增白色点云偏转问题记录

## 会话：2026-06-29（Jazzy 迁移）

### 构建兼容性修复
- ✅ `rmoss_gazebo/`：ignition-* → gz-* 包名迁移（从 `/home/lmy/桌面/rm_jazzy/` 同步）
- ✅ `rmoss_gz_resources/`：模型文件 + env-hook 更新（IGN_GAZEBO_RESOURCE_PATH → GZ_SIM_RESOURCE_PATH）
- ✅ `rmoss_interfaces/`：CMakeLists 更新（从参考同步）
- ✅ `pb2025_robot_description/`：env-hook + package.xml 模型路径更新
- ✅ `hero_localization/CMakeLists.txt`：`project(LANGUAGES C CXX)` + `find_package(MPI)` 修复 VTK/MPI 编译错误
- ✅ `rmoss_gz_bridge/CMakeLists.txt`：从 Humble 合并 rfid_bridge target
- ✅ `rfid_bridge_node.*`：ignition → gz 头文件/命名空间迁移

### 启动兼容性修复
- ✅ `gazebo.launch.py`：gz_version `6→8`（匹配 Gazebo Harmonic）
- ✅ `spawn_robots.launch.py`：`ign service` → `gz service`
- ✅ `rmu_gazebo_simulator/`：从 Humble 同步，仅改 gz_version

### 点云反复横跳修复
- ✅ RawLidar：切换 Cyclone DDS (`rmw_cyclonedds_cpp`)，C++ BEST_EFFORT relay 替代 Python relay
- ✅ RegisteredScan：全链路 depth 增大（loam_adapter 5→50, Point-LIO 20→100, RViz 5→100）
- ✅ Cyclone DDS 配置：`/home/lmy/cyclonedds.xml`，绑定 lo 接口

### 当前状态
| 组件 | 状态 |
|------|------|
| 全部 19 个包编译安装 | ✅ |
| 仿真启动（Gazebo + RViz + Point-LIO） | ✅ |
| RawLidar 点云显示 | ✅ 稳定 |
| RegisteredScan 点云显示 | ✅ 稳定 |
| TF 链 map→odom→base→chassis→muzzle | ✅ |
| Cyclone DDS | ✅ 已配置 |
| 系统 UDP 缓冲 (25MB) | ✅ |
| 模型 mesh 显示 | ✅ |
