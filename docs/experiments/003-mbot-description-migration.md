# 实验 003:mbot 机器人模型迁移(阶段 2b)

- 日期:2026-08-14
- 阶段:阶段 2b(仿真核心:mbot 模型加载)
- 结果:✅ 通过

## 目标

将 mbot 两轮小车模型从经典 Gazebo 迁移到 ROS2 Jazzy + Gazebo Harmonic,
模型能正常生成、落位,结构(link/joint)正确。

## 改动内容

| 文件 | 改动 |
|------|------|
| `src/sentry_gazebo_2024/m_bot/mbot_description/package.xml` | 新建 ament_cmake 包 |
| `src/sentry_gazebo_2024/m_bot/mbot_description/CMakeLists.txt` | 安装 urdf/ |
| `urdf/mbot.xacro` | 顶层入口,实例化 mbot_base 宏 |
| `urdf/mbot_base.xacro` | 合并显示版 + 仿真版结构 |

关键改动:
1. 合并 `mbot_base.xacro`(显示,无碰撞/惯量)与 `mbot_base_gazebo.xacro`(仿真,含碰撞/惯量);
2. **去除**经典 Gazebo 插件(diff_drive、ros_control)与 `<transmission>`——这些留到 2c 用 gz_ros2_control 重加;
3. 保留完整 link/joint 结构:base_footprint、base_link、左右轮、前后万向轮(共 6 link + 5 joint)。

## 验证结果

```bash
colcon build --symlink-install --packages-select mbot_description   # ✅
xacro mbot.xacro | grep -c "<link "    # 6
xacro mbot.xacro | grep -c "<joint "   # 5

# gz-sim 生成
gz sim -s -r .../rmuc_static.world &
ros2 run ros_gz_sim create -file mbot.urdf -name mbot -x 5 -y 8 -z 0.1
# => Entity creation successful.  ✅
# => 模型列表: RMchangdi1230, mbot  ✅
# => 位姿 [5, 8, -0.000593]: 从 z=0.1 落位到场地表面(碰撞生效) ✅
```

## 备注

- 生成方式用 `ros_gz_sim create`(URDF→SDF 自动转换)。
- 后续 2c 需要:gz_ros2_control 差速控制器 + 传输;2d 需要:gpu_lidar 雷达。
- 观察到 sdformat 转换把 base_link 的惯量合并到了 base_footprint(固定关节链),
  质量/惯量总量正确,不影响物理;若影响 2c 控制器再处理。
