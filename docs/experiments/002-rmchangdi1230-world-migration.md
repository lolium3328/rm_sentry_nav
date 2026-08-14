# 实验 002:场地模型迁移到 Gazebo Harmonic(阶段 2a)

- 日期:2026-08-14
- 阶段:阶段 2a(仿真核心:RMUC 世界启动)
- 结果:✅ 通过

## 目标

将 `RMchangdi1230` 场地(世界 + 网格)从经典 Gazebo 迁移到 ROS2 Jazzy 的
Gazebo Harmonic(gz-sim 8.11),并能在无头模式下干净启动。

## 改动内容

| 文件 | 改动 |
|------|------|
| `src/rmchangdi1230/package.xml` | 新建 ament_cmake 包 |
| `src/rmchangdi1230/CMakeLists.txt` | 安装 worlds/ 与 meshes/ 到 share/ |
| `worlds/rmuc_static.world` | 迁移并清理(见下) |
| `meshes/base_link.STL` | 原样复制(51MB,仅用于视觉) |

世界文件关键改动:
1. 物理引擎:经典 Gazebo 的 `ode` → gz-sim 的 `dartsim`;
2. 移除旧 `<state>` 快照(残留的 sim_time)与旧 `<gui>` 相机块;
3. 网格路径由 `../../RMchangdi1230/...` 改为包内相对路径 `../meshes/...`;
4. **碰撞简化**(关键):去掉 51MB STL 碰撞,改用简化地面 plane。

## 踩坑记录

1. **51MB STL 不能做碰撞网格**:
   - dartsim 报 `Mesh construction from an SDF has not been implemented yet` + `collision couldn't be created`;
   - bullet-featherstone 加载该 STL 直接卡死(12s 无输出);
   - 结论:碰撞用简化几何,视觉保留完整 STL(激光雷达 gpu_lidar 走渲染、
     看的是视觉几何,不受影响)。
2. **gz sim 输出缓冲**:`timeout ... > file` 重定向时若进程被 kill,缓冲日志会丢失;
   改用 `| head` / `| grep` 管道能正常刷出。验证命令注意用管道。

## 验证结果

```bash
colcon build --symlink-install --packages-select rmchangdi1230   # ✅
gz sim -s -r <install>/share/rmchangdi1230/worlds/rmuc_static.world
# => World [default] initialized with [1ms] physics profile.  ✅
# => 无 collision couldn't be created 错误 ✅
```
