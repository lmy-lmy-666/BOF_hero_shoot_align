# 进度日志

## 会话：2026-06-27

### 阶段 1：需求与架构确认
- **状态：** complete
- 探索 ITL_Hero_Shoot 项目结构
- 读取 RM 2026 规则手册（英雄 42mm 关键参数）
- 确认导航组职责：只做定位，交付 TF树 + target_x + target_h
- 确认架构：原创 hero_* 包 + 拷贝仿真依赖

### 阶段 2-4：项目搭建、算法、节点
- **状态：** complete
- 创建 5 个原创包：hero_core、hero_interfaces、hero_localization、hero_description、hero_bringup
- 11 个 gtest 用例全部通过
- 3 个 ROS2 节点编译通过
- 从 ITL_Hero_Shoot 拷贝仿真依赖包

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
- ✅ 探索 /home/lmy/ITL_Hero_Shoot/ 参考项目完整结构
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
