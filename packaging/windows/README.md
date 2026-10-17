# Windows 发布

每次发布提供三种 x64 文件，用户任选一种：

- `VisionCraft-版本-windows-x64.msi`：Windows Installer 安装包，可选择安装目录，创建开始菜单入口，支持系统“已安装的应用”卸载和后续版本升级。
- `VisionCraft-版本-windows-x64-setup.exe`：内嵌同一 MSI 的启动程序，适合直接双击安装。与 MSI 共用安装身份，不应重复安装两份。
- `VisionCraft-版本-windows-x64-portable.exe`：免安装，运行时把程序和依赖解压到临时目录，退出清理；设置持久保存在 EXE 旁的 `VisionCraft-data/`。请放在可写目录，搬迁时连同数据目录一起移动。首次启动包含解压时间。

运行依赖 Qt、OpenCV、fmt 和 MSVC CRT 随包部署，目标电脑无需安装开发环境。安装版的个人设置继续使用 Qt 的系统用户配置位置，卸载程序不删除用户导出的图片和工程。

## 发布流水线

Windows runner 使用 Qt 的 `windeployqt` 收集 QML、插件及依赖，再补充 OpenCV、vcpkg 和 app-local MSVC CRT。WiX 4.0.6 生成 MSI，NSIS 生成安装 EXE 与便携 EXE；发布附件不使用 ZIP。

`generate_wix.py` 根据部署目录生成完整的文件清单，文件组件 GUID 按相对路径稳定生成，产品 UpgradeCode 固定。MSI 管理安装、升级与卸载；EXE 安装入口只启动内嵌 MSI。

`package.ps1 -Payload dist -Version 0.2.0` 执行打包，版本参数由 CI 从根 CMakeLists.txt 读取。需要 Python、WiX 4.0.6（含 UI 扩展）和 NSIS。
