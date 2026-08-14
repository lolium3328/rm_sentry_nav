# AGENTS.md — 项目开发规则

本文件是本仓库的开发规范,所有 AI 编码助手(AGENTS)与开发者必须遵守。

## 项目概述

哨兵导航 **ROS1(Noetic)→ ROS2(Jazzy)** 迁移项目。
开发环境由 Docker 管理(Ubuntu 24.04 + ROS2 Jazzy + OCS2),见 `docker/`。

## 目录结构

```
.
├── docs/          # 文档:迁移计划、实验结果(markdown)
├── docker/        # Docker 环境:Dockerfile、docker-compose、entrypoint
├── legacy/        # 原 ROS1 代码(只读参考,独立 git 仓库)
└── src/           # 新 ROS2 工作区(colcon),按功能包分文件夹
```

## 开发规则

1. **提交规范**:每个逻辑改动单独 commit,message 使用约定式提交(`feat:` / `fix:` / `docs:` / `chore:` / `refactor:`)。

2. **工作区规范**:根目录保持干净,按功能分文件夹管理代码层级,不散落文件。

3. **代码规范**:单文件不超过 500 行,超过必须拆分(头文件/实现分离、按职责拆分)。

4. **Python 项目**:统一使用 `uv` 管理依赖与虚拟环境。

5. **前端项目**:统一使用 TypeScript + Bun。

6. **文档规范**:所有计划与实验结果保存为 markdown,放入 `docs/`(实验放 `docs/experiments/`)。

## 迁移工作流

1. 按 `docs/migration-plan.md` 的阶段顺序迁移。
2. 改完一个包,在容器内 `colcon build --packages-select <pkg>` 验证。
3. 验证结果记录到 `docs/experiments/`。
4. 验证通过后 commit。

## 环境命令

```bash
# 进入开发环境(镜像已构建:sentry:jazzy)
docker compose -f docker/docker-compose.yml run --rm sentry

# 容器内编译单个包
cd /root/sentry_ws && colcon build --symlink-install --packages-select <pkg>
```
