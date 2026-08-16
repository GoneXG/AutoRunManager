# main.cpp —— Win32 图形界面主程序

## 作用
GUI 程序入口，负责：
- 解析命令行参数（`--run` 提权启动 / 文件路径注册 / 无参数打开主窗口）
- 创建主窗口：已注册文件列表（ListView）+「注册新文件」「注销」按钮
- 打开“注册新文件”对话框并完成注册

## 关键流程

### 1. 程序入口 `wWinMain`
- 用 `GetCommandLineW()` + `CommandLineToArgvW()` 解析**完整**命令行，并**跳过 argv[0]**。
  原因：命令行为空时 `CommandLineToArgvW` 会把 exe 自身路径填入 argv[0]，
  不跳过会导致程序误把自己当成要注册的文件。
- `--run <路径>`（开机提权模式）：程序凭管理员清单被系统拉起后，
  调用 `ac::LaunchTarget()` 以管理员身份启动目标文件，然后退出。
- 其他任意参数视为文件路径：以默认“管理员权限”注册该文件。
- 无参数：初始化公共控件 → 注册窗口类 → 创建主窗口 → 进入消息循环。

### 2. 主窗口 `MainWndProc`
- `WM_CREATE`：
  - ListView（报表样式，两列：文件绝对路径 / 运行权限；整行选中、网格线）
  - 「注册新文件」「注销」按钮（初始禁用“注销”）
  - `RefreshList()` 载入已注册条目
- `WM_SIZE`：按窗口大小重排列表与按钮（按钮在右下角）
- `WM_COMMAND`：
  - 「注册新文件」→ `RunRegisterDialog()`，成功后 `RefreshList()`
  - 「注销」→ 取选中项 → 确认框 → `ac::UnregisterById()` → 刷新
- `WM_NOTIFY`（`LVN_ITEMCHANGED`）：存在选中项时启用“注销”按钮

### 3. 注册对话框 `RegDlgProc` / `CreateRegDlg` / `RunRegisterDialog`
| 控件 | 作用 |
|---|---|
| 静态文本「选择文件：」 | 标签 |
| Edit | 路径输入框，可手动输入 |
| Button「▼」 | 点击调用 `BrowseForFile()`（IFileOpenDialog）选文件并回填路径 |
| 静态文本「运行权限：」 | 标签 |
| ComboBox | “管理员权限 / 普通权限”，默认选中“管理员权限” |
| 「注册」 | 校验路径 → `ac::RegisterEntry()` → 成功关闭并刷新主列表 |
| 「取消」 | 关闭对话框 |

- `BrowseForFile()`：COM 初始化 + `IFileOpenDialog`，开启
  `FOS_FILEMUSTEXIST / FOS_PATHMUSTEXIST / FOS_FORCEFILESYSTEM`，
  用 `SIGDN_FILESYSPATH` 取回绝对路径。
- `RunRegisterDialog()`：`EnableWindow(parent, FALSE)` 禁用主窗口实现模态，
  `IsDialogMessage` 消息循环，窗口销毁后恢复主窗口并置前。

## 注意
- 界面中文以 `\uXXXX` 转义写在宽字符串字面量中，源码保持纯 ASCII，避免编码问题。
- “注册”前先 `ac::FileExists()` 校验，路径无效弹出“文件不存在或路径无效”。