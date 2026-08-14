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
└── src/           # 新 ROS2 工作区(colcon)
```

**src/ 内部按功能分组,分组层级与 legacy/ 保持一致**,便于逐文件对照:

```
src/
├── sentry_gazebo_2024/   # 仿真相关(对应 legacy sentry_gazebo_2024)
│   ├── rmchangdi1230/    # 场地(对应 legacy RMchangdi1230)
│   └── m_bot/            # mbot 系列(对应 legacy m_bot)
├── sentry_planning/      # 规划相关(对应 legacy sentry_planning)
└── sentry_msgs/          # 消息(对应 legacy rm2023_sentry_msgs)
```

> 包目录名按 ROS2 规范小写,但分组目录名与 legacy 保持一致。

## 开发规则

1. **提交规范**:每个逻辑改动单独 commit,message 使用约定式提交(`feat:` / `fix:` / `docs:` / `chore:` / `refactor:`)。

2. **工作区规范**:根目录保持干净,按功能分文件夹管理代码层级,不散落文件。**迁移到 src/ 的目录层级必须与 legacy/ 原项目保持一致**(如 sentry_gazebo_2024/、sentry_planning/ 分组),便于逐文件对照审核。

3. **代码规范**:新编写或实质重构的单文件不超过 500 行,超过必须拆分(头文件/实现分离、按职责拆分)。从 `legacy/` 迁移的原文件如果本身已超过 500 行,可保留原结构,不强制为了迁移而拆分；如后续对其进行实质重构,再按职责拆分。

4. **Python 项目**:统一使用 `uv` 管理依赖与虚拟环境。

5. **前端项目**:统一使用 TypeScript + Bun。

6. **文档规范**:所有计划与实验结果保存为 markdown,放入 `docs/`(实验放 `docs/experiments/`)。
7. **计划文档分层**:`docs/migration-plan.md` 是总计划与阶段索引,保持简洁稳定；阶段详细规划、实施拆分与验收标准必须单独保存为 `docs/phase-<n>-plan.md`，不得直接大段改写总计划。

## 迁移工作流

1. **迁移可追溯**: 迁移到 `src/` 的每个文件顶部,注明原 ROS1 参考文件路径(`legacy/...`),便于审核对照。
2. 按 `docs/migration-plan.md` 的阶段顺序迁移，并参考对应的独立阶段规划文档。
3. 改完一个包,在容器内 `colcon build --packages-select <pkg>` 验证。
4. 验证结果记录到 `docs/experiments/`。
5. 验证通过后 commit。

## 环境命令

```bash
# 进入开发环境(镜像已构建:sentry:jazzy)
docker compose -f docker/docker-compose.yml run --rm sentry

# 容器内编译单个包
cd /root/sentry_ws && colcon build --symlink-install --packages-select <pkg>
```
