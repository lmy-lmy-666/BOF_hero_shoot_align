# SLAM 建图流程 — 生成先验点云地图 (prior PCD map)

## 为什么需要建图

重定位（relocalization）需要一张先验点云地图，让机器人知道自己在地图上的位置。
这个地图是**录制的一段场地 LiDAR 点云**，通常在场地上无人时录制。

## 前提条件

- [ ] 机器人 LiDAR + IMU 正常工作
- [ ] ROS 2 环境已 source
- [ ] Point-LIO 已 clone 到 workspace src/ 并编译通过

## 步骤

### 1. 启动 SLAM（建图模式）

```bash
# 启动 Point-LIO 的建图 launch（不发布map TF，纯粹记录点云）
ros2 launch point_lio mapping_mid360.launch.py
```

如果 Point-LIO 包没有自带的 mapping launch，修改其默认 launch 文件添加 pcd_save：

```yaml
# 在 Point-LIO 配置中添加：
pcd_save:
  pcd_save_en: true
  interval: -1    # -1 表示结束时保存一次
```

### 2. 遥控机器人扫描场地

- 用遥控器或键盘控制机器人在场地上**缓慢走一圈**
- 覆盖所有以后可能需要定位的区域（包括狙击点附近）
- **尽量走直线、少急转**，IMU 在剧烈旋转下漂移更大
- 建议录制 **2-3 分钟**的点云

### 3. 停止并保存

```bash
# Ctrl+C 停止 Point-LIO
# Point-LIO 会在退出时自动保存 PCD 到运行目录
# 默认文件名类似: GlobalMap.pcd
```

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
- [ ] Z 轴没有太大偏差（假设平坦地面）
- [ ] 点云密度在关键区域足够

### 5. 部署地图

```bash
# 复制到 hero_shoot 项目
cp GlobalMap.pcd /path/to/hero_shoot/pcd/Hero.pcd

# 在 robot_params.yaml 中指定路径:
relocalization:
  ros__parameters:
    prior_pcd_file: "/path/to/hero_shoot/pcd/Hero.pcd"
```

## 常见问题

| 问题 | 可能原因 | 解决 |
|------|---------|------|
| 点云稀疏 | 障碍物少、场地空旷 | 多走几圈，贴墙走 |
| 重影严重 | SLAM 漂移、回环失败 | 降低速度、启用回环检测 |
| Z 轴漂移 | IMU 初始化不好 | 启动时保持静止 2-3 秒 |
| 地图太大 | 录制时间过长 | 用 pcl 降采样：`pcl_voxel_grid -leaf 0.1 GlobalMap.pcd Hero.pcd` |

## 进阶：多地图策略

如果场地很大或定位精度要求高，可以考虑：
- **分区建图**：将场地分成 2-3 个区域分别建图
- **启动时选图**：根据机器人初始位置自动选择最近的地图
- **动态更新**：比赛期间偶发性的点云更新（需要额外开发）
