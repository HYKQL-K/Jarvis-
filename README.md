<h1 align="center">Jarvis 助理工程仓库 :sparkles:</h1>
[English README](README_EN.md)

## 项目简介
精简版语音/多模态助理工程：包含 Unity 应用、原生 C/C++ 底层、TypeScript 包、模型清单与自动化脚本。仓库已移除臃肿的 Unity TextMesh Pro 示例资源与本地模型文件，只保留核心场景和代码。

## 推荐环境
- OS：Windows 10/11
- Node.js 18+，pnpm（用 corepack 启用），Git
- CMake ≥ 3.26，Ninja
- Visual Studio Build Tools（含 C++ 工作负载，使用 “x64 Native Tools Command Prompt” 编译）
- Unity：版本见 `apps/unity/ProjectSettings/ProjectVersion.txt`
- 工具：curl（用于脚本下载模型）

## 目录结构
- `apps/`：运行时应用（`unity`）
- `native/`：原生层（`include/jarvis`、`src`、`cmake`）
- `packages/`：共享包（`core-agent`、`rag`、`sdk` 等）
- `models/`：模型存放目录（仓库不含模型文件，运行脚本或手动放置）
- `assets/`：媒体与提示词
- `configs/`、`scripts/`、`tests/`、`benchmarks/`、`.github/workflows/`

## 架构图
```mermaid
graph TD
  ui[Unity app (apps/unity)]
  core[Core (@jarvis/core-agent)]
  rag[RAG (@jarvis/rag)]
  native[Native (C/C++)]
  models[Models (models/*)]
  tools[Tools (packages/sdk + tools)]
  assets[Assets (assets/*)]

  ui -->|RPC/events| core
  core --> rag
  core --> native
  native --> models
  core --> tools
  core --> assets
```

## 零基础运行步骤（PC/Windows）
1) 准备环境：安装推荐工具；若缺 VS 环境，请使用 “x64 Native Tools Command Prompt for VS”。
2) 打开终端，切到仓库根目录。
3) 安装依赖：
   ```bash
   pnpm install
   ```
4) 下载模型（仓库不带模型，脚本会联网）：
   ```bash
   ./scripts/get_models.sh
   ```
   - 如果需要换成可访问的真实地址，编辑脚本内的 URL 再运行。
   - 也可手动将模型放入 `models/` 下对应子目录，保持 manifest 路径一致。
5) 构建原生层：
   ```bash
   cmake -S native -B native/build -G Ninja
   cmake --build native/build
   ```
   若遇到标准头缺失，请在 VS 开发者命令行中执行。
6) 运行核心测试：
   ```bash
   pnpm -C packages/core-agent test
   ```
7) 运行 Unity 场景：用 Unity 打开 `apps/unity`，双击 `Scenes/Demo.unity`，点击 Play。
8) 桌面打包：Unity > File > Build Settings，目标 Windows x86_64，Add Open Scenes，输出到 `build/desktop`。
9) 烟测（可选）：`./scripts/run_smoke_demo.sh`，查看 `smoke.log` 是否包含 wake/ASR 行。
10) Stub 脚本：`scripts/build_android.sh`、`scripts/build_desktop.ps1` 为示例，不会产出正式包。

### 下载与模型说明
- 仓库不包含任何模型文件，`models/` 已加入 `.gitignore`。
- 推荐使用脚本拉取；如网络受限，手动下载后放入 `models/<name>/` 并更新 `manifest.json`。
- 保持 manifest 中的文件名与实际模型一致，否则加载会失败。

### 一键命令速查
```bash
pnpm install
./scripts/get_models.sh
cmake -S native -B native/build -G Ninja && cmake --build native/build
pnpm -C packages/core-agent test
```

### 常见问题（点击展开）
<details>
  <summary>pnpm 找不到 / 安装失败 :thinking:</summary>
  运行 <code>corepack prepare pnpm@latest --activate && corepack enable</code>，再执行 <code>pnpm install</code>。
</details>
<details>
  <summary>Unity 版本不匹配 :gear:</summary>
  查看 <code>apps/unity/ProjectSettings/ProjectVersion.txt</code>，用 Unity Hub 安装对应版本打开。
</details>
<details>
  <summary>模型下载受限 :link:</summary>
  编辑 <code>scripts/get_models.sh</code> 替换为可访问 URL；或手动下载到 <code>models/</code>，保持 manifest 文件指向正确路径。
</details>
<details>
  <summary>原生构建缺少标准头 :hammer:</summary>
  在 “x64 Native Tools Command Prompt for VS” 中执行 cmake，确保 MSVC 环境变量已加载。
</details>
