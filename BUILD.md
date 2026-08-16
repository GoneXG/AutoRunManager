# AutoStart Manager —— 开机自启动管理工具（C++ / Win32）

一个使用 C++（Win32 API）编写的 Windows 开机自启动注册/管理程序。

- 支持命令行传参：`AutoRunManager.exe "C:\path\to\file.exe"` 直接注册该文件的开机自启动
- 程序默认以管理员权限运行（清单 `requireAdministrator`）
- 自启动项写入当前用户 Run 注册表键，可在 **任务管理器 → 启动** 中看到并直接启用/禁用
- 图形界面：
  - 主窗口：已注册文件列表（绝对路径 + 运行权限）+「注册新文件」「注销」按钮
  - 注册窗口：「选择文件」路径栏（可手输，或点击右侧 ▼ 打开文件选择器）+「运行权限」下拉框（管理员权限 / 普通权限）+「注册」按钮

## 目录结构

```
.
├─ src/
│  ├─ main.cpp               # Win32 GUI 主程序（主窗口 + 注册窗口）
│  ├─ autorun_core.h         # 注册表核心逻辑头文件
│  ├─ autorun_core.cpp       # 注册 / 注销 / 枚举 / 提权启动逻辑
│  ├─ app.manifest           # 正式清单（requireAdministrator + Common-Controls v6）
│  ├─ app_debug.manifest     # 调试清单（asInvoker，免 UAC，仅供测试）
│  ├─ resource.rc            # 资源脚本（内嵌清单 + 版本信息）
│  └─ resource_debug.rc      # 调试版资源脚本
└─ build_msvc.bat            # 一键构建脚本（x64）
```

## 环境要求

- Windows 7 及以上（按 Windows 10/11 兼容清单编译，x64）
- Visual Studio Build Tools（含 C++ 桌面开发组件，提供 `cl.exe` / `rc.exe` / `link.exe`）
- Windows SDK（提供 `rc.exe` / `mt.exe`）

> 构建脚本中写死了本机示例路径：
> `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat`
> 如果你的 VS/SDK 路径不同，请修改 `build_msvc.bat` 第一行的 `vcvars64.bat` 路径。

## 构建方法

### 方式一：一键脚本（推荐）

```bat
build_msvc.bat
```

脚本依次执行：

1. 调用 `vcvars64.bat` 初始化 x64 编译环境
2. `rc.exe` 编译资源脚本（内嵌应用程序清单与版本信息）
3. `cl.exe` 编译并链接，产出三个程序到 `build\` 目录：

| 产物 | 说明 |
| --- | --- |
| `build\AutoRunManager.exe` | **正式版** GUI 程序（管理员权限清单） |
| `build\AutoRunManager_debug.exe` | 调试版 GUI（asInvoker，免 UAC，仅供界面测试） |

### 方式二：手动命令

在 **x64 Native Tools 命令行**（或先执行 `vcvars64.bat`）中运行：

```bat
cd src

:: 1. 编译资源（内嵌清单 + 版本信息）
rc /nologo resource.rc

:: 2. 编译并链接正式版 GUI 程序
cl /nologo /W3 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE main.cpp autorun_core.cpp resource.res ^
   /Fe:..\build\AutoRunManager.exe /link /SUBSYSTEM:WINDOWS /MANIFEST:NO ^
   user32.lib gdi32.lib comctl32.lib ole32.lib advapi32.lib shell32.lib shlwapi.lib uuid.lib

```

### 关键编译参数说明

| 参数 | 作用 |
| --- | --- |
| `/DUNICODE /D_UNICODE` | 使用 Unicode（宽字符）API |
| `/utf-8` | 源码按 UTF-8 解析（中文宽字符串字面量正确转换） |
| `/SUBSYSTEM:WINDOWS` | GUI 子系统（正式版） |
| `/SUBSYSTEM:CONSOLE` | 控制台子系统（自测程序） |
| `/MANIFEST:NO` | 禁止链接器自动生成清单，统一使用资源中内嵌的清单 |
| `/EHsc` | C++ 异常处理 |

## 运行与使用

- **图形界面**：双击 `AutoRunManager.exe`（UAC 确认后以管理员运行）
  1. 点击「注册新文件」
  2. 「选择文件」栏输入绝对路径，或点击右侧 ▼ 打开文件选择器
  3. 在「运行权限」下拉框中选择“管理员权限”或“普通权限”
  4. 点击「注册」，主窗口列表自动刷新
  5. 在列表中单击选中某行，点击「注销」即可移除该自启动项
- **命令行注册**（默认“管理员权限”）：
  ```bat
  AutoRunManager.exe "C:\path\to\your\program.exe"
  ```

### 注册表位置

| 内容 | 位置 |
| --- | --- |
| 自启动项 | `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`（值名以 `ShiAR_` 开头） |
| 注册信息（路径、权限） | `HKCU\Software\ShiAutoRunMgr\Entries` |

## 注意事项

- “管理员权限”自启动项在开机时先运行本程序（弹出 UAC 确认），再以管理员身份启动目标文件；**注册该类项后请勿移动或删除本程序**，否则该项将无法生效。
- “普通权限”自启动项直接启动目标文件，不依赖本程序位置。
- 目标为 `.exe/.com/.bat/.cmd` 时直接运行；为文档等其他类型文件时，通过系统默认关联程序打开。