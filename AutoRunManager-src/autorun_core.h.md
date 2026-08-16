# autorun_core.h —— 注册表核心逻辑头文件

## 作用
声明与注册表、自启动项相关的数据结构和接口，供 `main.cpp`（GUI）和
`selftest.cpp`（自测）复用。

## 内容
- 权限常量：
  - `kPermNormal = 0`：普通权限
  - `kPermAdmin  = 1`：管理员权限
- 数据结构 `Entry`：
  - `id`：内部条目 id（路径哈希，8 位十六进制）
  - `path`：文件绝对路径
  - `perm`：运行权限
- 对外接口：
  - `GetExePath()`：当前 exe 绝对路径
  - `NormalizePath()`：转为绝对路径（GetFullPathName + GetLongPathName）
  - `FileExists()`：路径存在且不是文件夹
  - `IsElevated()`：当前进程是否已提权（TokenElevation）
  - `RegisterEntry(path, perm, err)`：注册自启动项
  - `UnregisterById(id, err)` / `UnregisterByPath(path, err)`：注销
  - `ListEntries()`：枚举本程序注册过的全部条目
  - `ReadRunValue(id)`：读取 Run 键中的启动命令（调试/自测用）
  - `PermName(perm)`：权限显示名（管理员权限 / 普通权限）
  - `LaunchTarget(path, err)`：以当前进程权限启动目标文件