# rm_sentry_nav

哈工大 I Hiter 哨兵导航:**ROS1(Noetic)→ ROS2(Jazzy)** 迁移仓库。

## 目录

| 目录 | 说明 |
|------|------|
| `docs/` | 迁移计划与实验结果(markdown) |
| `docker/` | 开发环境(Ubuntu 24.04 + ROS2 Jazzy + OCS2) |
| `legacy/` | 原 ROS1 代码(只读参考) |
| `src/` | 新 ROS2 工作区(colcon, 按功能分组与 legacy 对照) |

```
src/
├── sentry_gazebo_2024/   # 仿真(场地、mbot、雷达...)
├── sentry_planning/      # 规划(全局/局部规划、目标点...)
└── sentry_msgs/          # 自定义消息
```

## 快速开始

```bash
# 1. 进入开发环境(需已安装 Docker + NVIDIA Container Toolkit)
docker compose -f docker/docker-compose.yml run --rm sentry

# 2. 容器内编译
cd /root/sentry_ws && colcon build --symlink-install
```

## 开发规范

见 [AGENTS.md](./AGENTS.md) 与 [迁移计划](./docs/migration-plan.md)。
