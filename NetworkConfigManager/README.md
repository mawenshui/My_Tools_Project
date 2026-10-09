# NetworkConfigManager

> Windows 桌面网络配置管理工具 | v2.2.2 | 2026-10-09

## 项目简介

NetworkConfigManager 是一个 Windows 桌面应用程序，用于便捷管理网络接口的 IP 配置。支持为同一网卡保存多套网络配置（静态 IP 或 DHCP），一键快速切换，适合需要在不同网络环境间频繁切换的场景（如办公/测试/现场调试）。

## 核心功能

- **网络配置管理**：新增、修改、删除、应用多套 IP 配置（静态 IP / DHCP）
- **网卡管理**：查看网卡状态，启用/禁用网络接口
- **快捷切换**：系统托盘右键菜单、桌面悬浮球，无需打开主窗口即可切换配置
- **系统集成**：支持开机自启动、最小化到系统托盘
- **视觉反馈**：配置应用成功/失败的动画反馈、托盘图标状态变化
- **暗色主题**：内置 Nord 风格暗色界面主题
- **配置版本管理**：支持配置历史记录和回滚功能
- **日志导出**：支持导出运行日志文件
- **操作进度反馈**：显示操作进度和步骤指示
- **性能监控**：记录关键操作耗时统计
- **网卡备份与回滚**：应用配置前保存当前网卡设置，支持恢复最近备份
- **批量配置**：对多块网卡批量应用保存的配置或恢复 DHCP
- **网段扫描**：扫描 IPv4 区间/CIDR 和指定端口，以进度条和状态提示显示扫描进度，查看主机名、MAC 与开放端口
- **网络诊断**：自动选择有网关和 DNS 的网卡进行延迟与丢包检测；使用 Windows WinHTTP 按地区或自定义 HTTP(S) 地址下载测速

国外测速预设使用 Cloudflare 50 MB 下载接口和 Hetzner FSN1 官方测试文件。Hetzner 使用单路下载，避免多路并发触发服务端限流；也可输入自定义 HTTP(S) 地址。
- **实时流量**：主页面显示所选网卡的上下行速率，并可打开系统网络连接窗口

## 技术栈

| 项目 | 说明 |
|------|------|
| **开发语言** | C++14 |
| **UI 框架** | Qt 5.12.12 (Widgets) |
| **构建工具** | qmake |
| **编译器** | MinGW 7.3.0 64-bit |
| **目标平台** | Windows (x64) |
| **核心依赖** | Qt Core, Qt Gui, Qt Widgets, Qt Network, Qt Concurrent, DbgHelp, Iphlpapi |

## 快速开始

### 环境要求

- Windows 7 及以上操作系统
- Qt 5.12+ (MinGW 64-bit)
- 管理员权限（应用配置时需要）

### 构建

```bash
qmake NetworkConfigManager.pro
mingw32-make
```

构建产物位于 `bin/NetworkConfigManager.exe`。发布构建建议使用独立目录；`package.ps1` 读取 `build_auto/app/bin/NetworkConfigManager.exe`。

### 运行

```bash
# 正常运行
bin/NetworkConfigManager.exe

# 最小化到托盘启动（适用于开机自启）
bin/NetworkConfigManager.exe --minimized
```

主页面「网络工具」区可直接打开批量配置、备份回滚、网段扫描、延迟检测、下载测速和系统网络连接；所选网卡的实时流量显示在同一区域。批量静态配置可能产生 IP 冲突，操作前会提示。配置和备份存于程序目录的 `config/`；安装版默认安装到当前用户可写目录。实际修改网卡仍需管理员权限。

### 自动化验证与打包

在 Qt 5.12.12 MinGW 64 位环境执行：

```powershell
Push-Location tests
$env:QT_QPA_PLATFORM = 'offscreen'
qmake configmanager_test.pro
mingw32-make release
.\release\configmanager_test.exe -txt
qmake networkscan_selfcheck.pro
mingw32-make release
.\release\networkscan_selfcheck.exe
qmake ui_selfcheck.pro
mingw32-make release
.\bin\ui_selfcheck.exe
Pop-Location
```

`tests/networkscan_selfcheck.pro` 验证网段/端口解析、扫描进度及本地 TCP 扫描；`tests/ui_selfcheck.pro` 验证主页入口、网卡自动选择、测速预设和本地 HTTP 下载测速。两个自检程序使用 `QT_QPA_PLATFORM=offscreen` 运行；设置 `NETWORKCONFIGMANAGER_TEST_HTTPS=1` 可额外测试公网 HTTPS 数据传输，`NETWORKCONFIGMANAGER_TEST_URL` 可指定测试地址。自动化验证不修改真实网卡；IP/DNS 应用、网卡启停和备份恢复需要在隔离的管理员测试机验收。

发布时先将已验证的 Release 可执行文件构建到 `build_auto/app/bin/`，再执行 `package.ps1 -QtBin <Qt的bin目录> -Iscc <ISCC.exe路径>`。脚本调用 `windeployqt`，生成 `releases/NetworkConfigManager-Setup-2.2.2-win64.exe` 与免安装 ZIP，并输出 SHA-256。安装版支持开始菜单、可选桌面快捷方式和卸载。

## 项目结构

```
NetworkConfigManager/
├── bin/                        # 构建输出
│   ├── config/                 # 配置数据文件
│   ├── logs/                   # 运行日志
│   └── NetworkConfigManager.exe
├── images/                     # 图标与图片资源
├── styles/                     # QSS 样式表
├── docs/                       # 项目文档
│   ├── requirements.md         # 需求文档
│   ├── design.md               # 设计文档
│   ├── specification.md        # 项目规范
│   └── optimization.md          # 可优化项分析
├── main.cpp                    # 程序入口
├── mainwindow.h / .cpp / .ui   # 主窗口
├── configmanager.h / .cpp      # 配置管理器
├── networkinterfacemanager.h / .cpp  # 网络接口管理器
├── floatwindow.h / .cpp        # 桌面悬浮窗
├── Logger.h                    # 日志系统
├── resources.qrc               # 资源索引
└── NetworkConfigManager.pro    # 项目构建配置
```

## 文档导航

| 文档 | 说明 |
|------|------|
| [需求文档](docs/requirements.md) | 功能需求、用户场景、非功能需求 |
| [设计文档](docs/design.md) | 系统架构、模块设计、核心流程 |
| [项目规范](docs/specification.md) | 编码规范、命名约定、构建发布流程 |
| [可优化项分析](docs/optimization.md) | 底层调用、实现方式、界面、悬浮球优化建议 |

## 许可证

内部项目，作者 mws。
