# SLAM 建图流程 — 生成先验点云地图 (prior PCD map)

## 为什么需要建图

重定位（relocalization）需要一张先验点云地图，让机器人知道自己在地图上的位置。
这个地图是**录制的一段场地 LiDAR 点云**，通常在场地上无人时录制。

## 前提条件

- [ ] 工作空间已编译（`colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release`）
- [ ] ROS 2 环境已 source（`source install/setup.bash`）

---

## 仿真建图

### 1. 启动仿真 SLAM

```bash
cd /home/lmy/BOF_hero_shoot_align
source install/setup.bash

ros2 launch hero_bringup hero_bringup.launch.py slam:=true
```

这会启动 Gazebo 仿真 + Point-LIO，`pcd_save.pcd_save_en` 自动设为 `true`，定位节点（loam_adapter、relocalization、target_computer）不启动。

### 2. 遥控机器人扫描场地

在另一个终端：

```bash
source /opt/ros/jazzy/setup.bash
ros2 run teleop_twist_keyboard teleop_twist_keyboard \
  --ros-args -r cmd_vel:=/red_standard_robot1/cmd_vel
```

- 在场地上**缓慢走一圈**，覆盖所有以后可能需要定位的区域
- **尽量走直线、少急转**，IMU 在剧烈旋转下漂移更大
- 建议录制 **2-3 分钟**的点云

### 3. 停止并保存

Ctrl+C 停止 launch，Point-LIO 退出时自动保存 PCD 到运行目录（默认文件名 `GlobalMap.pcd`）。

### 4. 检查地图质量

```bash
# 用 pcl_viewer 查看
pcl_viewer GlobalMap.pcd

# 或用 CloudCompare（GUI，更直观）
cloudcompare GlobalMap.pcd
```

检查要点：
- [ ] 场地轮廓清晰可见（墙壁、障碍物）
- [ ] 没有明显重影（多次经过同一位置时点云重叠一致）
- [ ] Z 轴没有太大偏差
- [ ] 点云密度在关键区域足够

### 5. 部署为默认地图

```bash
# 复制到 hero_bringup/pcd/（覆盖默认地图）
cp GlobalMap.pcd /home/lmy/BOF_hero_shoot_align/src/hero_bringup/pcd/Hero.pcd
```

之后仿真定位模式会默认加载这张地图。

---

## 实车建图

### 1. 启动雷达驱动（终端 1）

```bash
cd /home/lmy/BOF_hero_shoot_align
source install/setup.bash

ros2 launch livox_ros_driver2 msg_MID360_launch.py frame_id:=front_mid360 publish_freq:=10.0
```

### 2. 启动建图（终端 2）

```bash
cd /home/lmy/BOF_hero_shoot_align
source install/setup.bash

ros2 launch hero_bringup hero_bringup.launch.py mode:=robot slam:=true
```

启动后 RViz 自动加载 `visualize_robot_slam.rviz`（Fixed Frame: `camera_init`）。

### 3. 遥控机器人扫描场地

用遥控器控制机器人在场地上缓慢走一圈，覆盖所有需要定位的区域。

### 4. 停止并保存

Ctrl+C 停止 launch，地图自动保存为 `GlobalMap.pcd`。

### 5. 部署

```bash
cp GlobalMap.pcd /home/lmy/BOF_hero_shoot_align/src/hero_bringup/pcd/Hero.pcd
```

---

## 常见问题

| 问题 | 可能原因 | 解决 |
|------|---------|------|
| 点云稀疏 | 障碍物少、场地空旷 | 多走几圈，贴墙走 |
| 重影严重 | SLAM 漂移 | 降低速度、少急转 |
| Z 轴漂移 | IMU 初始化不好 | 启动时保持静止 2-3 秒 |
| 地图太大 | 录制时间过长 | 用 pcl 降采样：`pcl_voxel_grid -leaf 0.1 GlobalMap.pcd Hero.pcd` |
| 仿真建图失败 | sim_adapter 包未安装 | 确认 `src/sim_adapter/` 存在并已编译 |

## 地图降采样（可选）

如果生成的地图太大，影响 GICP 匹配速度：

```bash
pcl_voxel_grid -leaf 0.1 GlobalMap.pcd Hero.pcd
```

`-leaf` 值越大点越稀疏、匹配越快，建议 0.05~0.2。

## 进阶：多地图策略

如果场地很大或定位精度要求高，可以考虑：
- **分区建图**：将场地分成 2-3 个区域分别建图
- **启动时指定地图**：`ros2 launch hero_bringup hero_bringup.launch.py prior_pcd_file:=/path/to/map.pcd`
- **动态更新**：比赛期间偶发性的点云更新（需要额外开发）
