# AutoStart Manager —— 开机自启动注册/管理工具

> 基于 C++（Win32 API）编写的 Windows 开机自启动注册与管理系统，
> 注册项会显示在 **任务管理器 → 启动** 中，可随时启用/禁用。

## ✨ 功能特性

- **注册自启动**：将指定文件注册为开机自启动项（写入当前用户注册表 Run 键）
- **运行权限**：每条自启动项可设置“管理员权限”或“普通权限”
- **开机可见**：注册项显示在 **任务管理器 → 启动** 栏目中，可直接启用/禁用
- **图形管理**：主界面列表显示已注册文件的绝对路径与运行权限，选中后可一键注销
- **默认提权**：程序默认以管理员权限运行（清单 `requireAdministrator`）

## 🚀 快速开始

### 图形界面（推荐）

1. 双击 `AutoRunManager.exe`（UAC 确认后以管理员身份运行）
2. 点击「注册新文件」：
   - “选择文件”栏可手动输入绝对路径，也可点击栏位右侧的 **▼** 打开文件选择器
   - 在“运行权限”下拉框中选择“管理员权限”或“普通权限”
   - 点击「注册」，主界面列表自动刷新
3. 在主界面列表中单击选中一行，点击「注销」即可移除该自启动项

### 命令行

```bat
AutoRunManager.exe "C:\path\to\your\program.exe"
```

> 命令行注册默认使用“管理员权限”。

## 🗂️ 文档导航

按类别浏览本仓库保留的 Markdown 文档：

- **📖 构建说明**
  - [BUILD.md](BUILD.md) —— 环境要求、一键/手动构建步骤、编译参数与产物说明
- **💻 源码解析**（`AutoRunManager-src\` 目录）
  - [main.cpp.md](AutoRunManager-src/main.cpp.md) —— Win32 GUI 主程序：入口、主窗口、注册对话框
  - [autorun_core.h.md](AutoRunManager-src/autorun_core.h.md) —— 注册表核心逻辑头文件：数据结构与接口声明
  - [autorun_core.cpp.md](AutoRunManager-src/autorun_core.cpp.md) —— 注册/注销/枚举/提权启动的实现
- **🛠️ 构建脚本**
  - [build_msvc.bat.md](AutoRunManager-src/build_msvc.bat.md) —— MSVC 一键编译脚本说明

| 分类 | 文档 | 说明 |
| --- | --- | --- |
| 构建 | [BUILD.md](BUILD.md) | 构建过程与产物 |
| 源码 | [main.cpp.md](AutoRunManager-src/main.cpp.md) | GUI 主程序 |
| 源码 | [autorun_core.h.md](AutoRunManager-src/autorun_core.h.md) | 核心头文件 |
| 源码 | [autorun_core.cpp.md](AutoRunManager-src/autorun_core.cpp.md) | 核心实现 |
| 脚本 | [build_msvc.bat.md](AutoRunManager-src/build_msvc.bat.md) | 构建脚本 |

## 📁 目录结构

```
outputs/
├─ AutoRunManager.exe       # 可执行程序（x64，默认管理员权限）
├─ README.md                # 本文件（项目说明 + 文档导航）
├─ BUILD.md                 # 构建过程说明
└─ AutoRunManager-src/      # 源码 + 各源码文件的解析文档
   ├─ main.cpp              # GUI 主程序
   ├─ autorun_core.h/.cpp   # 注册表核心逻辑
   ├─ app.manifest / resource.rc 等
   └─ *.md                  # 对应的源码解析文档
```

## 🔧 工作原理

| 内容 | 位置 |
| --- | --- |
| 自启动项 | `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`（值名以 `ShiAR_` 开头） |
| 注册信息（路径、权限） | `HKCU\Software\ShiAutoRunMgr\Entries` |

- “管理员权限”项：开机时先运行本程序（弹出 UAC 确认），再以管理员身份启动目标文件
- “普通权限”项：开机时直接启动目标文件，不依赖本程序位置

## ⚠️ 注意事项

- 注册“管理员权限”项后，请**勿移动或删除**本程序，否则该项将无法生效
- 目标为 `.exe/.com/.bat/.cmd` 时直接运行；为文档等其他类型文件时，通过系统默认关联程序打开
- 注销请使用本程序界面完成（会同时清理注册表 Run 值与注册信息）
- 也可在“任务管理器 → 启动”中禁用某项（不影响本程序列表显示）

## 🖥️ 环境要求

- Windows 7 及以上（按 Windows 10/11 兼容清单编译，x64）
- 首次运行需要管理员权限（UAC 确认）