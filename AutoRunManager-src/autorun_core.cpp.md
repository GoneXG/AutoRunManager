# autorun_core.cpp —— 注册表核心逻辑实现

## 常量
| 常量 | 值 | 说明 |
|---|---|---|
| `kRunKey` | `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` | 任务管理器“启动”栏可显示/开关 |
| `kStoreKey` | `HKCU\Software\ShiAutoRunMgr\Entries` | 本程序私有注册信息 |
| `kValuePrefix` | `ShiAR_` | Run 键值名前缀 |

## 关键实现

### MakeId（路径哈希）
对路径小写后做 FNV-1a 32 位哈希，转成 8 位十六进制作为条目 id。
同一文件重复注册会更新同一条目（幂等），避免重复。

### RegisterEntry（注册）
1. `NormalizePath()` 转绝对路径；校验文件存在且不是文件夹。
2. 写元数据到 `Entries\<id>`：`Path`（REG_SZ）、`Perm`（REG_DWORD）。
3. 构造 Run 键启动命令：
   - 管理员权限：`"<本程序>" --run "<目标>"`
     （开机时先启动本程序，凭管理员清单提权后再拉起目标）
   - 普通权限 + `.exe/.com/.bat/.cmd`：直接 `"<目标>"`
   - 普通权限 + 其他类型（文档等）：`cmd.exe /c start "" "<目标>"`（经系统关联程序打开）
4. 写入 Run 键：值名 `ShiAR_<id>`，值数据为上述命令。

### UnregisterById（注销）
删除 Run 键值 `ShiAR_<id>`，再用 `RegDeleteTreeW` 删除 `Entries\<id>` 子键。

### ListEntries（枚举）
枚举 `Entries` 下所有子键，读取 `Path` / `Perm`，组装 `std::vector<Entry>`。

### LaunchTarget（提权启动）
`ShellExecuteW(open)` 启动目标；在 `--run` 模式下本程序已是管理员权限，
目标文件随之以管理员权限运行。