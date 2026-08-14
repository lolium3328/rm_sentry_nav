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
| `src/sentry_gazebo_2024/rmchangdi1230/package.xml` | 新建 ament_cmake 包 |
| `src/sentry_gazebo_2024/rmchangdi1230/CMakeLists.txt` | 安装 worlds/ 与 meshes/ 到 share/ |
| `worlds/rmuc_static.world` | 迁移并清理(见下) |
| `meshes/base_link.STL` | 原样复制(51MB,仅用于视觉) |

世界文件关键改动:
1. 物理引擎:经典 Gazebo 的 `ode` → gz-sim 的 `dartsim`;
2. 移除旧 `<state>` 快照(残留的 sim_time)与旧 `<gui>` 相机块;
3. 网格路径由 `../../RMchangdi1230/...` 改为包内相对路径 `../meshes/...`;
4. **碰撞简化**(关键):去掉 51MB STL 碰撞,改用简化地面 plane。

## 踩坑记录

1. **误导性 debug 日志(重要教训)**:
   dartsim 打印 `Mesh construction from an SDF has not been implemented yet for dartsim`,
   易被误读为"dartsim 不支持 STL 碰撞"。**实际上碰撞正常创建**——
   该日志只是说直接构造路径没实现,后面通过 `AttachMeshShapeFeature` 照常完成。
   官方 PR #452 已将其从 error 降级为 debug。
   验证方法:向场地上方丢一个球(半径 0.5,z=5),12 秒后球停在 z=0.59
   (球心 0.59 = 球底 0.09,落在场地表面),证明碰撞生效、未穿透。
2. **gz sim 输出缓冲**:`timeout ... > file` 重定向时若进程被 kill,缓冲日志会丢失;
   改用 `| head` / `| grep` 管道能正常刷出。验证命令注意用管道。
3. **物理引擎选择**:gz-sim 忽略 SDF 的 `<physics type>` 属性(官方示例仍写 `type="ode"`),
   实际引擎由命令行 `--physics-engine` 决定,默认 dartsim。

## 验证结果

```bash
colcon build --symlink-install --packages-select rmchangdi1230   # ✅
gz sim -s -r <install>/share/rmchangdi1230/worlds/rmuc_static.world
# => World [default] initialized with [1ms] physics profile.  ✅
# => 落球测试: 球从 z=5 落到 z=0.59, 碰撞正常, 未穿透 ✅
```
