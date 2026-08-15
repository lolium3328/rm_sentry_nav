# legacy 保真迁移审查

- 审查日期：2026-08-15
- 审查范围：`b47072c` 至 `21954af` 的迁移代码提交
- 对照基线：`legacy/` 当前 ROS1 源码
- 结论：阶段一基本保真；阶段二存在结构合并和参数变化；阶段三是简化重写，不能视为完成。

## 1. 审查标准

本次审查区分两类改动：

1. ROS2 必要适配：catkin→ament、roscpp→rclcpp、消息类型限定、QoS、TF2、
   Gazebo Classic→Harmonic、资源路径和 ROS2 强制命名规范。
2. 功能或结构变更：替换算法、删除分支、合并或改名功能文件、改变参数默认值、
   topic/message 语义或忽略输入数据。第二类改动没有明确批准时判定为不符合保真迁移。

编译、topic 连通和 Gazebo 端到端运行只作为可执行性证据，不替代逐文件和关键逻辑审查。

## 2. 总体结论

| 阶段 | 相关提交 | 结论 | 主要原因 |
|---|---|---|---|
| 阶段一 `sentry_msgs` | `b47072c`、`5a4d781` | 基本符合 | 字段保留，改动主要是 ROS2 类型限定和强制命名 |
| 阶段二场地 | `d3ef4e1`、`5c3c553`、`354b36b` | 当前主体恢复，仍需差异确认 | 曾用平面替代 STL 碰撞，后已恢复；SDF 仍有结构和物理配置变化 |
| 阶段二 mbot | `20e522e`、`9b598b5`、`354b36b` | 不符合新规则 | 合并原 xacro 文件和宏，部分参数及接口发生变化 |
| 阶段三规划 | `b24d8ec`、`547cb6c`、`21954af` | 不通过 | 用 4 组新实现替代原有 9 组核心实现，Topo、动态地图、优化平滑和完整 FSM 等缺失 |

`49b0a17` 只把已迁移包移动到与 legacy 对应的功能分组，未发现业务逻辑变化；
`91c9182` 增加 RViz2 雷达显示配置，属于新增验证配置，没有替代 legacy 功能。Docker、
实验记录和其他纯文档提交不包含 legacy 业务实现，不作算法保真判定。

## 3. 分项证据

### 3.1 阶段一：消息接口

- `RobotStatus.msg`、`RobotsHP.msg` 只将 `Header` 限定为 `std_msgs/Header`，业务字段未变。
- `slaver_speed.msg` 改为 `SlaverSpeed.msg`，字段未变；PascalCase 是 ROS2 接口命名要求。
- `GoTarget.srv` 字段未变。
- `trajectory_generation/msg/trajectoryPoly.msg` 在 `b24d8ec` 中迁到
  `sentry_msgs/msg/TrajectoryPoly.msg`。字段及系数数组布局保留，`time` 改为
  `builtin_interfaces/Time`；但消息所属包从 `trajectory_generation` 变为 `sentry_msgs`，
  这不是 ROS2 强制要求，会改变完整类型名，后续重迁时必须明确保留还是批准该映射。

结论：阶段一既有消息本身可以保留；`TrajectoryPoly` 的跨包移动列为待确认差异。

### 3.2 阶段二：场地

- `d3ef4e1` 删除了 `RMchangdi1230` 的 STL collision，并新增平面 collision。这是明确的
  几何和碰撞逻辑简化，不符合保真原则。
- `5c3c553` 在实测 dartsim 可加载 STL 后撤销上述简化，恢复同一份 STL 碰撞。当前
  legacy/src 的 `base_link.STL` SHA-256 一致：
  `87cde692fd24481381ecee5e26dfdb46fef75da051a5a873016f85186f4bd36b`。
- 当前 world 从 190 行变为 117 行。移除了保存态 `<state>`、注释掉的 ground plane、
  atmosphere/magnetic/spherical 配置和部分 contact 子项，同时将 ODE 改为 dartsim并加入
  Harmonic 系统插件。前两类多为静态快照或无效注释，后两类可能影响物理语义，不能仅凭
  “能启动”判为完全等价，重验收时应逐项确认。
- `world/` 改为 `worlds/` 不是 ROS2 强制要求。按新规则，后续整理时应恢复原相对目录，
  或单独记录并批准该映射。

结论：历史简化已经恢复；当前场地主体模型保留，但物理差异尚未完成保真确认。

### 3.3 阶段二：mbot 模型、驱动和雷达

- legacy 的显示模型 `urdf/xacro/mbot_base.xacro` 与仿真模型
  `urdf/xacro/gazebo/mbot_base_gazebo.xacro` 被合并为当前 `urdf/mbot_base.xacro`，
  `mbot_base_gazebo` 宏也被替换为 `mbot_base`。文件名、相对层级和宏入口没有保留。
- 经典 Gazebo diff-drive 插件替换为 Harmonic `DiffDrive` 属于必要后端适配，但参数并非
  一一映射：legacy `updateRate=100`，当前 `odom_publish_frequency=50`；命令 topic、TF、
  wheel torque/acceleration 等语义也没有逐项对照记录。
- gpu lidar 能输出 `1800×32` 点云，但实现被直接并入 `mbot_base.xacro`，没有保留 legacy
  gazebo/velodyne xacro 的同名模块边界。

结论：运行能力已验证，但结构和行为映射不完整，不能按“仅 ROS1→ROS2 适配”验收。

### 3.4 阶段三：`trajectory_generation`

legacy 的 9 个第一方 `.cpp` 共 4112 行，对应第一方头文件（不含 `backward.hpp` 和
`root_solver/`）共 994 行；当前 4 个 `.cpp` 和 4 个 `.hpp` 合计仅 717 行。行数本身不是
验收标准，但结合下列缺失项可确认这是替代实现，而不是原逻辑迁移。

原文件未同名迁移：

- `Astar_searcher.*`
- `RM_GridMap.*`
- `TopoSearch.*`
- `path_smooth.*`
- `plan_manager.*`
- `reference_path.*`
- `replan_fsm.*`
- `visualization_utils.*`
- `trajectory_generator_node.cpp`
- `node.h`、`backward.hpp`、`root_solver/cubic_spline.hpp`、`root_solver/lbfgs.hpp`

关键行为差异：

- `GridPlanner` 是新写的 2D 八邻域 A*，不是 `AstarPathFinder` 的原实现。
- `smooth_path()` 只做视线剪枝，替代了 Topo 搜索、原路径优化和基于 L-BFGS 的平滑。
- `ReferenceTrajectory` 使用新写的自然三次样条，替代 `reference_path`、`path_smooth`
  和 root solver 的原系数求解流程。
- `/points` 回调只打印一次日志，没有执行 `RM_GridMap.cpp` 中的点云过滤、局部占据、
  障碍膨胀、双层高度及动态地图更新。
- FSM 只保留 4 个简化状态和 4 个订阅，缺少裁判系统、轮速/航向、攻击目标及 legacy
  重规划条件分支。
- legacy visualization 创建约 13 组 Marker publisher；当前只发布一个路径 Marker。

因此 `docs/experiments/005-trajectory-generation-3a.md` 和 `006-trajectory-generation-3b.md`
仍可作为“简化实现能编译、能接收数据”的历史记录；
`007-trajectory-generation-phase3.md` 只能证明替代实现跑通，不能作为阶段三迁移完成证据。

### 3.5 计划和验收文档

- `b757124` 版本的阶段三计划写有“只迁移实际需要的纯算法代码”，并要求把超过 500 行的
  legacy 文件拆分。这两点会破坏逐文件对照，也与用户确认的保真迁移规则冲突。
- 本次已将 `docs/phase-3-plan.md` 改为全量运行时文件逐一映射，并允许 legacy 原有超长
  文件保留结构。
- 本次已在实验 007 增加后续审查结论，保留其运行链路数据，但撤销其“阶段三保真迁移
  通过”的效力。

## 4. 整改边界

1. 阶段三状态恢复为未完成，阶段四不得以当前实现为迁移基线。
2. 按 legacy 原文件名和相对层级逐文件迁移，保留原类、函数、算法和状态分支；超过
   500 行的原文件不拆分。
3. 当前 `grid_planner.*`、`reference_trajectory.*` 和合并式节点仅作为历史实验实现，
   不得继续扩展后冒充 legacy 对应模块。是否删除或保留到重迁完成时再做非破坏性决定。
4. 对确实无法直接使用的 ROS1/Gazebo API建立等价映射；任何无法等价实现的项先记录并
   请求批准，不自行删减。
5. 完成容器编译和端到端验证后，再增加文件映射、参数/topic 对照和关键算法输出对照，
   三类证据齐全才可重新标记阶段三完成。
