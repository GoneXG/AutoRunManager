# build_msvc.bat —— 一键构建脚本

## 作用
在 Windows 上使用 MSVC 工具链一键编译全部产物（正式版 / 调试版 / 自测程序）。

## 执行步骤
1. `call vcvars64.bat`：初始化 x64 编译环境（cl / rc / link 与 Windows SDK 路径）。
2. `rc /nologo resource.rc`：编译正式版资源（清单 + 版本信息）。
3. `cl ...` 链接出 `build\AutoRunManager.exe`
   - `/SUBSYSTEM:WINDOWS`（GUI）
   - `/MANIFEST:NO`（清单来自资源，不自动生成）
   - 依赖库：user32 / gdi32 / comctl32 / ole32 / advapi32 / shell32 / shlwapi / uuid
4. `cl ...` 链接出 `build\selftest.exe`（`/SUBSYSTEM:CONSOLE`）
5. `rc resource_debug.rc` + `cl ...` 链接出 `build\AutoRunManager_debug.exe`

## 注意
- 第一行的 `vcvars64.bat` 路径为示例，实际安装路径不同需自行修改。
- 依赖 Visual Studio Build Tools（含 C++ 桌面开发组件）与 Windows SDK。