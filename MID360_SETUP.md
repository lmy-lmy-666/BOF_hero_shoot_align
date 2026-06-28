# MID360 雷达连接与验证

## 硬件准备

| 物品 | 说明 |
|------|------|
| MID360 雷达 | 机身标签有 SN 码，本机 IP = `192.168.1.145` |
| Livox 航插一分三线 | 或自接线：网线 + 电源线（红正黑负） |
| 12V 直流电源 | 注意正负极，接错会烧 |

## 接线

```
┌──────────────┐
│   MID360     │── M12 航插 ──┬── 网线 ──── 电脑网口 (enp46s0)
└──────────────┘              └── 红线(+) ─ 12V 正极
                                └── 黑线(-) ─ 12V 负极
```

## 步骤

### 一、设置电脑 IP

```bash
# 把电脑网口 IP 设为 192.168.1.50（和雷达同一网段）
sudo ip addr add 192.168.1.50/24 dev enp46s0
```

> 如果报 `RTNETLINK answers: File exists`，说明已经设过，跳过即可。

### 二、插线上电

把网线插上电脑，雷达接 12V 电源上电。等几秒后检查：

```bash
ip addr show enp46s0
```

看到 `state UP` 和 `inet 192.168.1.50` 就是通了。

### 三、启动雷达驱动

```bash
cd /home/lmy/hero_shoot
source install/setup.bash

ros2 launch livox_ros_driver2 msg_MID360_launch.py \
  frame_id:=front_mid360 \
  publish_freq:=10.0
```

### 四、验证数据

另开终端：

```bash
source /home/lmy/hero_shoot/install/setup.bash

ros2 topic hz /livox/lidar     # CustomMsg 类型，topic hz 不兼容；用 ros2 topic echo /livox/lidar --no-arr --once 验证
ros2 topic hz /livox/imu       # 应输出 200Hz
```

### 五、启动定位管线

数据正常后：

```bash
cd /home/lmy/hero_shoot
source install/setup.bash

# 建图（只跑 Point-LIO，Ctrl+C 保存 scans.pcd）
ros2 launch hero_bringup hero_bringup.launch.py mode:=robot slam:=true

# 或者定位（需要先验地图）
ros2 launch hero_bringup hero_bringup.launch.py mode:=robot prior_pcd_file:=/path/to/map.pcd
```

## 故障排查

| 现象 | 可能原因 | 解决 |
|------|---------|------|
| 网口始终 `DOWN` | 网线没插好或雷达没上电 | 检查航插是否拧紧、电源是否接对 |
| lidar 无数据、驱动连接不上 | lidar IP 不匹配 | `sudo tcpdump -i enp46s0 -nn 'host 192.168.1.0/24'` 抓包确认雷达实际 IP，修改 `MID360_config.json` |
| `topic hz /livox/lidar` 报 multi-type 错 | CustomMsg 类型与 topic hz 不兼容 | 用 `ros2 topic echo /livox/lidar --no-arr --once` 替代 |
| 点云频率很低或为 0 | 雷达盲区遮挡 | 移开雷达前方 0.5m 内的物体 |
| 驱动报错退出 | 端口被占用 | `pkill livox_ros` 后重试 |

## 关键参数速查

| 参数 | 当前值 | 所在文件 |
|------|--------|---------|
| 雷达 IP | `192.168.1.145`（不同设备 IP 不同，抓包确认） | `src/livox_ros_driver2/config/MID360_config.json` |
| 电脑 IP | `192.168.1.50` | 手动设置 |
| frame_id | `front_mid360` | 启动时命令行指定 |
| 数据端口 | 56300 | `MID360_config.json` |
| IMU 端口 | 56400 | `MID360_config.json` |
