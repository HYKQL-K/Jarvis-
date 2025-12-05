<h1 align="center">Jarvis 助理工程仓库 ✨</h1>

<div align="center">

[English README](README_EN.md)

</div>

## 📖 项目简介
这是一个**精简版**的语音/多模态助理工程，旨在提供一个干净的开发底座。
项目包含 Unity 应用端、C/C++ 原生底层、TypeScript 核心包以及配套的自动化脚本。
> **注意**：为了保持轻量，我移除了臃肿的 Unity TextMesh Pro 示例资源和庞大的本地模型文件，仅保留了核心业务场景与基础代码架构。

## 💻 推荐开发环境
* **操作系统**: Windows 10 / 11
* **基础工具**: Node.js 18+ (需启用 corepack), pnpm, Git, curl
* **构建工具**: CMake ≥ 3.26, Ninja
* **编译器**: Visual Studio Build Tools (必须包含 **C++ 工作负载**，并从 "x64 Native Tools Command Prompt" 启动)
* **Unity**: 具体版本请查看文件 `apps/unity/ProjectSettings/ProjectVersion.txt`

## 📂 目录结构
* `apps/`: 运行时应用 (Unity 工程)
* `native/`: 底层核心库 (包含 `include/jarvis`, `src`, `cmake` 构建脚本)
* `packages/`: TypeScript 共享包 (如 `core-agent` 核心逻辑, `rag` 检索增强等)
* `models/`: **模型仓库** (默认这里是空的，需要你手动下载或配置脚本拉取)
* `assets/`, `configs/`, `scripts/` ... : 各类资源配置与自动化脚本

## 🏗️ 架构概览

```mermaid
graph TD
  ui["Unity App (apps/unity)"]
  core["Core (@jarvis/core-agent)"]
  rag["RAG (@jarvis/rag)"]
  native["Native (C/C++)"]
  models["Models (models/*)"]
  tools["Tools (packages/sdk + tools)"]
  assets["Assets (assets/*)"]

  ui -->|RPC/Events| core
  core --> rag
  core --> native
  native --> models
  core --> tools
  core --> assets
````

## 🚀 快速上手 (Windows)

1.  **准备环境**
    确保安装了上述所有工具。如果你没有完整的 VS IDE，请务必使用 **"x64 Native Tools Command Prompt for VS"** 命令行工具进行后续操作。

2.  **安装依赖**
    在仓库根目录下运行：

    ```bash
    pnpm install
    ```

3.  **配置并下载模型 (关键步骤\!)**

    > ⚠️ **重要**：仓库默认的 `scripts/get_models.sh` 中包含的是示例 URL (`example.com`)。

      * **方法 A (推荐)**: 编辑 `scripts/get_models.sh`，将里面的 URL 替换为你自己的下载地址或国内镜像源，然后运行脚本。
      * **方法 B (手动)**: 直接下载模型文件，放入 `models/<模型名>/` 目录，并确保文件名与目录下的 `manifest.json` 定义一致。

4.  **构建原生层**
    使用 CMake 编译 C++ 底层库：

    ```bash
    cmake -S native -B native/build -G Ninja
    cmake --build native/build
    ```

    *如果提示找不到标准头文件，请检查是否在 VS 的开发者命令行中运行。*

5.  **运行核心测试**
    确保核心逻辑正常工作：

    ```bash
    pnpm -C packages/core-agent test
    ```

6.  **Unity 预览**

      * 使用 Unity Hub 打开 `apps/unity` 目录。
      * 双击打开场景 `Scenes/Demo.unity`。
      * 点击 **Play** 按钮即可在编辑器内运行。

7.  **构建桌面应用**
    在 Unity 中选择 `File > Build Settings`，目标平台选 **Windows x86\_64**，点击 "Add Open Scenes"，构建路径设为 `build/desktop`。

8.  **冒烟测试 (可选)**
    运行 `./scripts/run_smoke_demo.sh`，检查生成的 `smoke.log` 日志中是否包含 `wake` (唤醒) 或 `ASR` (识别) 等关键词，以验证功能闭环。

### ⚠️ 关于打包脚本

`scripts/build_android.sh` 和 `scripts/build_desktop.ps1` 目前仅为**占位示例 (Stub)**，直接运行**不会**产出可用的正式安装包。请根据你的 CI/CD 需求自行完善这些脚本。

### 🔗 模型文件对照表 (示例)

请确保你下载的模型文件名与代码预期一致，否则会加载失败：

| 模型类型 | 建议文件名示例 | 备注 |
| :--- | :--- | :--- |
| **Wake Word** | `xiaobai.tflite` | 唤醒词模型 (OpenWakeWord) |
| **ASR (Small)** | `small-int8-zh.bin` | Faster-Whisper 量化版 |
| **TTS** | `mandarin.onnx` | Piper 语音合成模型 |
| **Embedding** | `bge-m3.onnx` | 向量化模型 |


