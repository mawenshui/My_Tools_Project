# NetworkConfigManager 设计文档

- **版本号**：v2.2.1
- **更新日期**：2026-10-09

---

## 1. 文档概述

本文档是 **NetworkConfigManager**（Windows 桌面网络配置管理工具）的架构与设计文档，详细描述了项目的系统架构、模块设计、核心流程、数据结构、关键设计决策及模块间交互关系。文档内容基于源码阅读确认，力求准确反映实际实现。

### 读者对象

- 项目维护者与二次开发者
- 技术评审人员
- 新加入的开发人员

---

## 2. 系统架构概览

### 2.1 分层架构

```
┌─────────────────────────────────────────────────────┐
│                    表 示 层                           │
│  MainWindow (QMainWindow + Qt Designer UI)           │
│  FloatWindow (无边框悬浮球)                            │
│  QSystemTrayIcon (系统托盘图标 + 右键菜单)              │
├─────────────────────────────────────────────────────┤
│                    业 务 层                           │
│  ConfigManager (配置 CRUD + 权限检测 + 配置应用)       │
│  NetworkInterfaceManager (netsh 命令封装)             │
├─────────────────────────────────────────────────────┤
│                    系 统 层                           │
│  Windows netsh 命令                                   │
│  Windows SID API (CheckTokenMembership)               │
│  Windows 注册表 (开机自启配置)                          │
│  Windows ShellExecuteEx (管理员提权)                   │
├─────────────────────────────────────────────────────┤
│                  基 础 设 施                           │
│  Logger (Header-only 单例日志系统)                     │
│  Qt 5.12.12 Framework (Signal/Slot、QProcess、QMutex) │
│  Windows SEH / MiniDump (崩溃处理)                     │
│  QSharedMemory (单实例锁)                              │
│  QSettings (窗口位置/状态持久化)                        │
│  JSON (配置持久化)                                     │
└─────────────────────────────────────────────────────┘
```

### 2.2 模块关系

```
                              ┌──────────────┐
                              │   main.cpp   │
                              │  (程序入口)   │
                              └──────┬───────┘
                                     │ 创建
                                     ▼
┌──────────────┐    ┌─────────────────────────────┐    ┌────────────────────────────┐
│  Logger.h     │◄───│          MainWindow          │───►│       ConfigManager        │
│(日志系统单例) │    │  - 配置列表 CRUD UI           │    │  - JSON 持久化              │
│              │    │  - 网卡管理控件               │    │  - 管理员权限检测 (SID API)  │
│              │    │  - 托盘菜单 + 快速配置菜单     │    │  - 提权 (ShellExecuteEx)    │
│              │    │  - 窗口状态持久化             │    │  - 配置验证 (正则)          │
│              │    │  - 单实例检查                 │    │  - netsh 命令执行           │
│              │    │  - 配置比较 + 视觉反馈        │    │  - 接口缓存 (30s TTL)       │
│              │    │  - QSS 暗色主题              │    │  - 线程安全 (QMutex)        │
│              │    │                              │    │                             │
│              │    │  持有:                        │    │  信号:                      │
│              │    │  - ConfigManager*             │    │  - configApplied()          │
│              │    │  - FloatWindow*               │    │  - errorOccurred()          │
│              │    │  - QSystemTrayIcon*           │    │  - adminStatusChanged()     │
│              │    │  - QSharedMemory              │    │                             │
│              │    ├──────────┬───────────────────┤    └──────────────┬─────────────┘
│              │    │          │                   │                   │ 调用
│              │    │  拥有    │  拥有              │  调用              │
│              │    ▼          ▼                   ▼                   ▼
│              │ ┌────────┐ ┌──────────────────┐ ┌──────────────────────────┐
│              │ │FloatWindow│ │ QSystemTrayIcon │ │NetworkInterfaceManager │
│              │ │ (悬浮球) │ │ (系统托盘)       │ │  - 全静态方法             │
│              │ │          │ │                  │ │  - QProcess 执行 netsh   │
│              │ │ 64x64    │ │ 右键菜单含:       │ │  - 3s 超时                │
│              │ │ 图片/自绘  │ │ · 快速应用配置   │ │  - GBK 编码 (fromLocal8Bit)│
│              │ │ 拖动     │ │ · 网卡管理       │ │  - 接口列表/状态/启用/禁用│
│              │ │ hover 效果│ │ · 开机自启       │ │                          │
│              │ │          │ │ · 显示/隐藏悬浮球 │ │  信号:                    │
│              │ │ 信号:    │ │ · 显示主窗口     │ │  (无自定义信号)            │
│              │ │·doubleClicked│ · 退出         │ │                          │
│              │ │·showContextMenu│               │ │                          │
│              │ └────────┘ └──────────────────┘ └──────────────────────────┘
│              │
│              │  使用 QSettings (posConfig.ini):
│              │  - FloatWindow 位置
│              │  - 窗口几何/状态
│              │  - 悬浮窗可见性
│              └─────────────────────────────┘
```

---

## 3. 技术选型说明

| 技术项 | 选型 | 理由 |
|-------|------|------|
| **GUI 框架** | Qt 5.12.12 | 成熟的跨平台 C++ GUI 框架，信号槽机制便于模块解耦 |
| **语言标准** | C++14 | 与 Qt 5.12.12 兼容良好，提供 auto、lambda、智能指针等现代特性 |
| **构建系统** | qmake | 与 Qt 5 原生捆绑，配置简洁，无需额外依赖 |
| **编译器** | MinGW 64-bit | Windows 平台 GCC 兼容编译器，与 Qt 官方预编译包匹配 |
| **日志方案** | Header-only 单例 Logger | 无编译链接额外依赖，静态便捷方法调用方便，线程安全 |
| **配置持久化** | JSON 文件 (config.json) | 人类可读，Qt 内置 QJsonDocument 原生支持，格式标准 |
| **窗口状态持久化** | QSettings (INI 格式) | Qt 原生跨平台方案，键值对简洁，位置/状态小数据场景适用 |
| **管理员权限检测** | Windows SID API | 精准确认当前进程是否在 Administrators 组中运行 |
| **管理员提权** | ShellExecuteEx + runas | Windows 标准提权方式，安全可靠，无需额外库 |
| **网络配置修改** | netsh 命令行 | Windows 原生网络配置接口，兼容性好，无需驱动/内核交互 |
| **单实例检查** | QSharedMemory | Qt 原生 IPC 机制，轻量，无需额外依赖 |
| **崩溃处理** | Windows SEH + MiniDumpWriteDump | 生成 minidump 文件供开发者诊断，记录崩溃日志 |
| **依赖库** | DbgHelp.lib | Windows SDK 自带，仅用于 minidump 生成 |
| **样式主题** | QSS (Qt Style Sheet) | Qt 原生样式表，支持暗色主题等自定义外观 |

---

## 4. 模块设计详解

### 4.1 main.cpp — 程序入口

**文件路径**：[main.cpp](../main.cpp)

#### 职责定位

程序启动入口，负责初始化运行时环境、异常处理体系、日志系统和 QApplication 实例，并创建主窗口。

#### 关键实现逻辑

1. **Windows SEH 异常处理** — 通过 `SetUnhandledExceptionFilter()` 注册 `GlobalExceptionHandler`，在发生 SEH 异常（如访问违规、除零）时，使用 `MiniDumpWriteDump()` 生成 `crash.dmp` 文件，并将错误码记录到 Logger。

2. **C++ 标准异常处理** — 通过 `std::set_terminate()` 注册 `CppExceptionHandler`，捕获 `std::exception` 和未知异常类型并记录。

3. **Qt 消息 → Logger 桥接** — 通过 `qInstallMessageHandler()` 将 Qt 内部的 `qDebug()`、`qWarning()`、`qCritical()`、`qFatal()` 等消息全部转发到 Logger 系统，实现日志统一管理。`QtFatalMsg` 触发后会额外弹出对话框并调用 `abort()`。

4. **内存泄漏检测** — 在 `_DEBUG` 模式下通过 `_CrtSetDbgFlag()` 启用 MSVC CRT 内存泄漏检测。

5. **启动模式控制** — 解析命令行参数，若包含 `--minimized` 则仅调用 `w.hide()`（程序在后台通过托盘运行），否则调用 `w.show()` 正常显示。

6. **日志配置** — 日志文件最大 10MB，保留 5 个备份文件。

#### 依赖关系

- 依赖：Logger.h、MainWindow
- 被依赖：无（作为程序入口，不被其他模块引用）

---

### 4.2 MainWindow — 主窗口（~1700 行）

**文件路径**：[mainwindow.h](../mainwindow.h) / [mainwindow.cpp](../mainwindow.cpp)
**UI 定义**：[mainwindow.ui](../mainwindow.ui)

#### 职责定位

应用程序的核心控制中心，负责：
- 配置列表 CRUD 的 UI 交互
- 配置应用触发与结果反馈
- 网卡管理界面（启用/禁用/刷新）
- 托盘图标与菜单管理
- 浮动窗口联动控制
- 窗口状态持久化与恢复
- 快速应用菜单的生成与更新
- 配置比较逻辑（避免重复应用）

#### UI 布局（基于 mainwindow.ui）

通过 Qt Designer 设计的 `QMainWindow`，尺寸 913×487，由 `QGridLayout` 组织：

```
┌─────────────────────────────────────────────────────┐
│  █ 网卡管理                                          │
│  [networkInterfaceCombo] [启用网卡] [禁用网卡] [刷新列表] │
├─────────────────────────────────────────────────────┤
│  █ 状态： [statusBar]                     [○指示灯]    │
├─────────────────────────────────────────────────────┤
│  █ 配置列表          │  █ 配置详情                     │
│  [configList]        │  网络接口: [interfaceCombo] [刷新]│
│  (QListWidget)       │  IP获取方式: ◎静态IP ○DHCP      │
│                      │  IP地址:    [ipEdit]           │
│                      │  子网掩码:  [subnetEdit]        │
│                      │  默认网关:  [gatewayEdit]       │
│                      │  DNS服务器: [primaryDnsEdit] 备用: [secondaryDnsEdit] │
│                      │  [添加配置] [更新配置] [删除配置] [应用配置] │
└─────────────────────────────────────────────────────┘
```

#### 关键成员变量

| 变量 | 类型 | 用途 |
|------|------|------|
| `ui` | `Ui::MainWindow*` | Qt Designer 生成的 UI 对象 |
| `m_configManager` | `ConfigManager*` | 配置管理器实例 |
| `m_quickMenu` | `QMenu*` | 托盘中的"快速应用配置"菜单 |
| `m_floatWindow` | `FloatWindow*` | 浮动悬浮窗实例 |
| `m_trayIcon` | `QSystemTrayIcon*` | 系统托盘图标 |
| `m_singleInstanceLock` | `QSharedMemory` | 单实例检查锁，键名为 `应用名称+PID` |
| `m_settings` | `QSettings*` | 窗口位置配置（INI 格式） |
| `m_statusIndicator` | `QLabel*` | 状态栏配置状态指示灯（16×16） |
| `m_floatVisible` | `bool` | 悬浮窗可见性状态 |
| `m_autostart` | `bool` | 开机自启状态 |
| `m_lastConfigSuccess` | `bool` | 上一次配置应用是否成功 |

#### 关键实现逻辑

**配置名称格式**：采用 `[网卡名] 配置名` 格式，如 `[以太网] 办公室网络`。添加配置时自动拼接网卡名前缀。

**配置比较（compareConfigs）**：在应用配置前比较当前系统配置与目标配置。对于 DHCP 模式仅比较 method 字段；对于静态 IP 模式比较 ip、subnet、gateway 三个关键字段。配置相同时跳过应用，避免重复的 netsh 调用。

**视觉反馈（showConfigResult）**：
- 成功：悬浮球显示绿色图标 → 托盘图标变绿 → 状态指示灯变绿 → 气泡通知 → 窗口闪烁动画 → 状态栏消息
- 失败：悬浮球显示红色图标 → 托盘图标变红 → 状态指示灯变红 → 气泡通知 → 窗口闪烁动画 → 状态栏消息
- 3 秒后通过 `QTimer::singleShot` 恢复默认外观

**快速应用菜单（updateQuickMenu）**：遍历所有网卡接口，为每个接口创建子菜单，第一项显示当前实际配置（置灰不可点击），下方列出该接口的已保存配置。已匹配当前配置的项置为 checked 状态。配置数超过 8 时追加"更多配置..."入口链接到主窗口。

**悬浮窗菜单（showFloatWindowMenu）**：与快速应用菜单结构类似，额外包含各网卡的"启用/禁用"操作项，以及"置顶显示"开关。

**网卡管理**：通过 `networkInterfaceCombo` 下拉框展示网卡列表，格式为 `网卡名 [管理状态|连接状态]`，如 `以太网 [已启用|已连接]`。启用/禁用按钮的状态由 `updateInterfaceControls()` 根据当前选中网卡的状态动态控制。

**单实例检查**：在构造函数中最早执行，使用 `QSharedMemory` 以 `APP_NAME + PID` 为键尝试创建共享内存段。若已存在则 attach 成功，弹出错误提示并退出。注意：键名含 PID 可能因每次 PID 不同而失去实际拦截效果——这是当前实现的一个已知局限性。

**关闭行为**：重写 `closeEvent`，调用 `hide()` 并 `event->ignore()`，实现关闭即最小化到托盘。程序在 `main()` 中已设置 `setQuitOnLastWindowClosed(false)`。

**窗口状态持久化**：
- 主窗口几何/状态：通过 `QSettings`（默认注册表路径）保存 `windowGeometry`、`windowState`
- 悬浮窗可见性与位置：通过 `QSettings(posConfig.ini)` 保存 `FloatWindow/pos`
- 恢复悬浮窗位置时包含屏幕边界检查（`screenGeo.contains`）

**暗色主题**：通过 QSS 文件 `:/styles/styles/drak_theme.qss` 加载并应用暗色主题样式。

**权限控制**：当 `adminStatusChanged(false)` 时，禁用"添加/更新/删除/应用配置"四个按钮（`disableAdminFunctions()`）；恢复管理员后重新启用。

#### 信号槽连接

| 发送者 | 信号 | 接收者 | 槽函数 |
|--------|------|--------|--------|
| `configList` (QListWidget) | `itemClicked` | MainWindow | `onConfigSelected` |
| `interfaceCombo` (QComboBox) | `currentIndexChanged(int)` | MainWindow | `onInterfaceChanged` |
| `dhcpRadio` (QRadioButton) | `toggled(bool)` | MainWindow | `onIpMethodToggled` |
| `addButton` / `updateButton` / `deleteButton` / `applyButton` | `clicked` | MainWindow | `onAddConfig` / `onUpdateConfig` / `onDeleteConfig` / `onApplyConfig` |
| `refreshButton` | `clicked` | MainWindow | `updateInterfaces` |
| `enableInterfaceBtn` / `disableInterfaceBtn` | `clicked` | MainWindow | `onEnableInterface()` / `onDisableInterface()` |
| `refreshInterfacesBtn` | `clicked` | MainWindow | `refreshNetworkInterfaces` |
| `m_configManager` | `configApplied` | MainWindow | Lambda（日志+状态栏+菜单刷新） |
| `m_configManager` | `errorOccurred` | MainWindow | Lambda（日志+弹窗） |
| `m_configManager` | `adminStatusChanged` | MainWindow | Lambda（按钮启用/禁用） |
| `m_floatWindow` | `doubleClicked` | MainWindow | `showNormal` |
| `m_floatWindow` | `showContextMenu` | MainWindow | `showFloatWindowMenu` |
| `m_trayIcon` | `activated` | MainWindow | `onTrayIconActivated` |
| `qApp` | `aboutToQuit` | MainWindow | `saveFloatWindowPosition` |
| `networkInterfaceCombo` | `currentTextChanged` | MainWindow | `on_networkInterfaceCombo_currentTextChanged` |

#### 依赖关系

- 依赖：ConfigManager、NetworkInterfaceManager、FloatWindow、Logger.h、mainwindow.ui
- 被依赖：main.cpp（创建实例）

---

### 4.3 ConfigManager — 配置管理器（~650 行）

**文件路径**：[configmanager.h](../configmanager.h) / [configmanager.cpp](../configmanager.cpp)

#### 职责定位

业务层的核心模块，负责网络配置的持久化存储、权限管理与配置实际应用到 Windows 系统的完整流程。

#### 对外接口

| 方法 | 返回值 | 说明 |
|------|--------|------|
| `loadConfigs()` | `bool` | 从 JSON 文件加载所有配置到内存 |
| `saveConfigs()` | `bool` | 将内存配置保存至 JSON 文件（含防重入保护） |
| `configs()` | `QMap<QString, QVariantMap>` | 获取所有配置的线程安全副本 |
| `addConfig(name, config)` | `bool` | 添加新配置（含重名校验） |
| `updateConfig(oldName, newName, config)` | `bool` | 更新配置（支持重命名） |
| `removeConfig(name)` | `bool` | 删除指定配置 |
| `applyConfig(config)` | `bool` | 应用配置到 Windows 网络接口 |
| `isAdmin()` | `bool` | 查询当前管理员状态 |
| `requestAdminPrivileges()` | `bool` | 请求管理员权限提权（重启进程） |
| `getNetworkInterfaces(useCache)` | `QStringList` | 获取网络接口列表（支持缓存） |
| `cleanInterfaceName(rawName)` | `static QString` | 静态工具方法：清理网卡名称 |
| `validateConfig(config)` | `bool` | 验证配置数据有效性 |

#### 信号

| 信号 | 参数 | 触发时机 |
|------|------|----------|
| `configApplied` | `bool success, QString message` | `applyConfig()` 执行完毕时 |
| `errorOccurred` | `QString error` | 各类操作发生错误时 |
| `adminStatusChanged` | `bool isAdmin` | 管理员权限状态发生变化时 |

#### 关键成员变量

| 变量 | 类型 | 用途 |
|------|------|------|
| `m_mutex` | `mutable QMutex` | 保护 m_configs 的线程安全互斥锁 |
| `m_configs` | `QMap<QString, QVariantMap>` | 配置数据内存存储 |
| `m_configFile` | `QString` | 配置文件的完整路径 |
| `m_cachedInterfaces` | `QStringList` | 网络接口名称缓存 |
| `m_lastInterfaceUpdate` | `QDateTime` | 缓存最后更新时间 |
| `m_isAdmin` | `bool` | 管理员权限状态 |
| `m_isSaving` | `bool` | 保存操作防重入标志 |

#### 关键实现逻辑

**JSON 持久化**：配置文件位于 `<程序目录>/config/config.json`。构造函数中自动创建 config 目录。存储格式为外层 JSON 对象，键为配置名，值为嵌套的配置 QVariantMap。加载时通过 `validateConfig()` 校验每一条配置的有效性，无效配置则跳过并警告。

**管理员权限检测**：使用 Windows SID API：
1. `AllocateAndInitializeSid()` 构造 Administrators 组的 SID
2. `CheckTokenMembership()` 检查当前进程令牌是否属于该组
3. 检测完成后通过 `FreeSid()` 释放 SID

**管理员提权**：使用 `ShellExecuteEx()` + `lpVerb = "runas"` 以管理员身份重启当前进程。通过 `GetModuleFileName()` 获取当前程序路径。若用户取消（`ERROR_CANCELLED`）则发出对应提示信号。

**配置验证（validateConfig）**：
- 必备字段检查：`interface`、`method`
- method 取值校验：必须为 `"dhcp"` 或 `"static"`
- 静态 IP 模式下：ip 和 subnet 为必填
- IP 格式校验：正则 `^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$`，每段 0–255 范围检查
- 子网掩码、网关、DNS 均为可选，但如果提供则必须符合 IP 格式

**配置应用（applyConfig）**：
1. 权限检查（非管理员直接返回失败）
2. 配置验证（调用 `validateConfig`）
3. 网卡名称清理（`cleanInterfaceName` 去除括号和特殊字符）
4. DHCP 模式：执行 `netsh interface ipv4 set address name="网卡" source=dhcp`，成功后执行 `netsh interface ipv4 set dns name="网卡" source=dhcp`
5. 静态 IP 模式：执行 `netsh interface ip set address name="网卡" static IP 掩码 [网关] 1`，然后设置主 DNS（`set dns static`），再设置备用 DNS（`add dns ... index=2`）。若无 DNS 指定则自动设为 DHCP DNS
6. 备用 DNS 或 DHCP DNS 设置失败不标记为整体失败（采用部分成功策略）
7. 成功后清空接口缓存，强制下次刷新

**接口缓存**：`getNetworkInterfaces()` 使用 30 秒 TTL 缓存（`interfaceCacheTimeout`）。在 TTL 内且 `useCache=true` 时直接返回缓存；否则通过 `getNetshOutput()` 重新查询 netsh 并刷新缓存。无可用接口时返回默认值列表。

**线程安全**：
- `configs()`、`addConfig()`、`updateConfig()`、`removeConfig()` 使用 `QMutexLocker` 保护
- `loadConfigs()` 和 `internalSaveConfigs()` 使用 `QMutexLocker` 保护
- `saveConfigs()` 使用 `m_isSaving` 布尔标志实现防重入

**网卡名称清理（cleanInterfaceName）**：移除括号及其内容（正则 `\$.*\$`），移除非字母数字下划线连字符的字符（正则 `[^\p{L}\p{N}\s_-]`），对"以太网"和"WLAN"做特殊映射。

#### 依赖关系

- 依赖：直接调用 Windows API（AllocateAndInitializeSid、CheckTokenMembership、ShellExecuteEx、GetModuleFileName）
- 使用 QProcess 执行 netsh 命令
- 被依赖：MainWindow

---

### 4.4 NetworkInterfaceManager — 网络接口管理器（~180 行）

**文件路径**：[networkinterfacemanager.h](../networkinterfacemanager.h) / [networkinterfacemanager.cpp](../networkinterfacemanager.cpp)

#### 职责定位

封装 Windows `netsh` 命令行工具，提供获取网络接口信息、启用/禁用网卡的功能。所有公开方法均为静态方法。

#### 对外接口（全静态方法）

| 方法 | 返回值 | 说明 |
|------|--------|------|
| `getNetworkInterfaces()` | `QStringList` | 获取所有网络接口名称列表 |
| `enableInterface(interfaceName)` | `bool` | 启用指定网络接口 |
| `disableInterface(interfaceName)` | `bool` | 禁用指定网络接口 |
| `getInterfaceStatus(interfaceName)` | `QString` | 获取接口综合状态，格式 `"管理状态\|连接状态"` |
| `getInterfaceAdminStatus(interfaceName)` | `QString` | 获取接口管理状态（"已启用" / "已禁用"） |
| `getInterfaceConnStatus(interfaceName)` | `QString` | 获取接口连接状态（"已连接" / "已断开连接"） |

#### 关键实现逻辑

**netsh 命令执行（executeNetshCommand）**：
- 使用 `QProcess` 启动 `netsh` 进程
- 参数通过 `QStringList` 传递（命令字符串按空格拆分）
- 合并标准输出和错误输出（`MergedChannels`）
- 超时限制 3000ms（`waitForFinished(3000)`）
- 编码处理：`QString::fromLocal8Bit()`（GBK 编码）
- 输出清理：`trimmed()` 去除首尾空白

**接口列表解析**：解析 `netsh interface show interface` 的输出。使用正则表达式匹配行首的"管理状态 | 连接状态 | 类型 | 网卡名"四列格式。网卡名称使用非贪婪匹配确保到行尾结束。

**状态查询**：`getInterfaceAdminStatus()` 和 `getInterfaceConnStatus()` 均通过解析 `netsh interface show interface` 输出实现。按行遍历，匹配目标网卡名所在行，分别提取第 0 列（管理状态）和第 1 列（连接状态）。英文状态自动转换为中文（`"Enabled"→"已启用"` 等）。

**启用/禁用网卡**：
- 执行 `netsh interface set interface "网卡名" enable/disable`
- 成功条件：输出为空 或 输出包含 "已经是启用状态"/"已经是禁用状态"
- 其他情况视为失败并记录错误日志

---

### 4.5 FloatWindow — 悬浮窗口（~320 行）

**文件路径**：[floatwindow.h](../floatwindow.h) / [floatwindow.cpp](../floatwindow.cpp)

#### 职责定位

提供可拖动、可自定义外观的悬浮窗口，作为配置应用的快捷入口和状态指示器。

#### 窗口属性

- 窗口标志：`FramelessWindowHint | WindowStaysOnTopHint | Tool`（无边框 + 置顶 + 工具窗口）
- 背景属性：`Qt::WA_TranslucentBackground`（透明背景）
- 固定尺寸：64×64 像素
- 默认文本：`"IP"`
- 默认背景色：`QColor(61, 219, 255)`（浅蓝色）

#### 对外接口

| 方法 | 说明 |
|------|------|
| `setBackgroundPixmap(pixmap)` | 设置背景图标（图片模式） |
| `clearBackgroundPixmap()` | 清除背景图标，恢复自绘制模式 |
| `hasBackgroundPixmap()` | 查询是否有背景图标 |
| `setBackgroundColor(color)` | 设置自绘制背景颜色 |
| `setText(text)` | 设置显示文本 |
| `setTextColor(color)` | 设置文本颜色 |
| `setDraggable(enabled)` | 设置是否可拖动 |

#### 信号

| 信号 | 参数 | 用途 |
|------|------|------|
| `doubleClicked()` | 无 | 双击时通知主窗口显示自身 |
| `showContextMenu(pos)` | `QPoint`（屏幕坐标） | 右键时通知主窗口显示上下文菜单 |

#### 绘制模式

支持两种绘制模式：

1. **图片模式**（`m_hasBackgroundPixmap == true`）：居中绘制缩放到窗口尺寸的背景图标。此模式常在配置应用结果展示时使用（绿色/红色图标切换）。

2. **自绘制模式**（`m_hasBackgroundPixmap == false`）：绘制圆形背景（`drawEllipse`，比窗口小 10px 的圆），圆心显示加粗 12pt 文本。悬停时背景色变亮 10%（`lighter(110)`）。

#### 交互逻辑

- **拖动**：鼠标左键按下记录起始偏移 `m_dragPosition`，移动时实时更新窗口位置
- **双击**：发出 `doubleClicked()` 信号
- **右键**：发出 `showContextMenu(globalPos)` 信号
- **悬停**：`enterEvent` 设置手形光标 + 背景高亮，`leaveEvent` 恢复默认光标 + 正常背景

---

### 4.6 Logger.h — 日志系统（~320 行 Header-only 单例）

**文件路径**：[Logger.h](../Logger.h)

#### 职责定位

提供全应用统一的线程安全日志记录能力，支持多级别、按日分割、自动轮转和多层目录容灾。

#### 日志级别

| 级别 | 枚举值 | 用途 |
|------|--------|------|
| DEBUG | 0 | 调试信息 |
| INFO | 1 | 普通信息 |
| WARNING | 2 | 警告信息 |
| ERROR | 3 | 错误信息 |
| CRITICAL | 4 | 严重错误 |

#### 对外接口（静态便捷方法）

| 方法 | 说明 |
|------|------|
| `Logger::instance()` | 获取单例指针 |
| `Logger::init(logDir, prefix)` | 初始化日志系统（文件命名、目录创建） |
| `Logger::debug(message)` | 记录 DEBUG 级别日志 |
| `Logger::info(message)` | 记录 INFO 级别日志 |
| `Logger::warning(message)` | 记录 WARNING 级别日志 |
| `Logger::error(message)` | 记录 ERROR 级别日志 |
| `Logger::critical(message)` | 记录 CRITICAL 级别日志 |
| `Logger::setMaxSizeMB(size)` | 设置日志文件最大尺寸（MB） |
| `Logger::setBackupCount(count)` | 设置保留备份数量 |
| `instance()->shutdown()` | 刷新缓冲区并关闭日志文件 |

#### 关键实现逻辑

**文件名格式**：`<prefix>_<yyyyMMdd>.log`，如 `app_20260601.log`

**目录容灾**：按优先级尝试多个日志目录：
1. 用户指定的 `logDir`（如果有）
2. `<当前工作目录>/logs/`
3. `<临时目录>/<应用名>/logs/`
4. 全部失败时回退到控制台输出（`stdout`）

文件打开失败时最多重试 500ms（每 10ms 尝试一次）。

**日志轮转**：当文件大小达到 `m_maxSizeBytes`（默认 5MB）时触发。轮转策略：
1. 删除最旧的备份文件（`.N`）
2. 将 `.N-1` 重命名为 `.N`，以此类推
3. 将当前日志重命名为 `.1`
4. 重新创建并打开当前日志文件

**日志格式**：`[yyyy-MM-dd hh:mm:ss.zzz] [级别] 消息内容\n`

**线程安全**：通过 `QMutex` 保护所有写操作。`log()`、`init()`、`rotateLog()`、`shutdown()` 均在操作前加锁。

**单例实现**：使用函数内 `static` 局部变量的方式（Meyer's Singleton），构造于首次调用 `instance()`。构造函数和拷贝构造均为 private 且删除。

---

## 5. 核心流程

### 5.1 启动流程

```
main() 入口
  │
  ├─► SetUnhandledExceptionFilter(GlobalExceptionHandler)   # SEH 异常处理
  ├─► std::set_terminate(CppExceptionHandler)               # C++ 异常处理
  ├─► _CrtSetDbgFlag() (DEBUG 模式)                         # 内存泄漏检测
  │
  ├─► QApplication 构造
  │   ├─ setApplicationName("NetworkConfigManager")
  │   ├─ setApplicationVersion("2.2.1")
  │   ├─ setQuitOnLastWindowClosed(false)                   # 关闭最后窗口不退出
  │   └─ qInstallMessageHandler(Qt→Logger)                  # Qt 消息桥接
  │
  ├─► Logger::instance()->init()                            # 日志系统初始化
  ├─► Logger::setMaxSizeMB(10)                              # 日志上限 10MB
  ├─► Logger::setBackupCount(5)                             # 保留 5 个备份
  │
  └─► MainWindow 构造
      ├─► checkSingleInstance()                             # QSharedMemory 单例检查
      │   ├─ attach 成功 → 弹窗报错 → qApp->quit()
      │   └─ create(1) 成功 → 继续
      │
      ├─► initSettings(posConfigPath)                       # QSettings 初始化
      │   └─ 确保 config/posConfig.ini 文件存在且可写
      │
      ├─► setupUi()                                         # UI 初始化
      │   ├─ restoreWindowState()                           # 恢复窗口位置/状态
      │   └─ 初始化状态指示灯（16×16 灰色图标）
      │
      ├─► initializeApplication()                           # 应用初始化
      │   ├─ loadConfigs()                                  # 加载 JSON 配置
      │   ├─ updateInterfaces()                             # 更新网卡列表
      │   ├─ refreshNetworkInterfaces()                     # 刷新网卡管理列表
      │   └─ 检查注册表开机自启状态
      │
      ├─► setupTrayIcon()                                   # 系统托盘初始化
      │   └─ 构建右键菜单（快速配置/网卡管理/自启/退出）
      │
      ├─► setupConnections()                                # 建立信号槽连接
      │
      ├─► loadFloatWindowPosition()                         # 恢复悬浮窗位置
      │
      ├─► loadAndApplyStyleSheet("drak_theme.qss")          # 加载暗色主题
      │
      └─► show()                                            # 显示主窗口
  │
  ├─► 检查 --minimized 参数
  │   ├─ 存在 → w.hide()                                   # 最小化到托盘
  │   └─ 不存在 → w.show()                                 # 正常显示
  │
  └─► a.exec()                                              # 进入事件循环
```

### 5.2 配置应用流程

```
用户点击 [应用配置] / 托盘菜单 / 悬浮窗菜单
  │
  ▼
onApplyConfig()
  │
  ├─► 检查 m_currentConfig 是否为空
  │   └─ 为空 → showConfigResult(false, "请先选择一个配置")
  │
  ├─► 从 m_configManager 获取当前配置的 QVariantMap
  │   └─ 为空 → showConfigResult(false, "无效的配置")
  │
  ├─► 检查网卡管理状态
  │   ├─ NetworkInterfaceManager::getInterfaceAdminStatus()
  │   └─ "已禁用" → showConfigResult(false, "网卡已被禁用")
  │
  ├─► 比较当前配置与目标配置
  │   └─ compareConfigs() == true → showConfigResult(true, "配置未变更")
  │
  └─► ConfigManager::applyConfig(config)
      │
      ├─► isAdmin() 检查
      │   └─ false → 返回 "需要管理员权限"
      │
      ├─► validateConfig(config)
      │   └─ false → 返回 "配置验证失败"
      │
      ├─► cleanInterfaceName() 清理网卡名称
      │
      ├─► method == "dhcp"
      │   ├─ netsh interface ipv4 set address source=dhcp
      │   └─ netsh interface ipv4 set dns source=dhcp
      │
      └─► method == "static"
          ├─ netsh interface ip set address static IP 掩码 [网关] 1
          ├─ netsh interface ip set dns static primary_dns
          └─ netsh interface ip add dns secondary_dns index=2
      │
      └─► emit configApplied(success, message)
  │
  ▼
MainWindow 收到 configApplied 信号
  ├─► showConfigResult(success, message)
  │   ├─ 切换悬浮窗背景（绿/红图标）
  │   ├─ 更新状态指示灯
  │   ├─ 窗口闪烁动画（500ms，透明度 1.0→0.7→1.0）
  │   ├─ 状态栏消息（3000ms）
  │   ├─ 更新托盘图标 + 气泡通知（3000ms）
  │   └─ 3 秒后恢复默认外观
  │
  └─► 延迟 500ms 后刷新快速菜单和悬浮窗
```

### 5.3 权限管理流程

```
权限检测：
  checkAdminStatus()
    ├─ AllocateAndInitializeSid()    # 构造 Administrators 组 SID
    ├─ CheckTokenMembership()        # 检查进程令牌
    ├─ FreeSid()                     # 释放 SID
    └─ 状态变化时 emit adminStatusChanged()

权限提权：
  requestAdminPrivileges()
    ├─ GetModuleFileName()           # 获取当前程序路径
    ├─ ShellExecuteEx(lpVerb="runas") # 管理员身份重启
    ├─ 用户确认 → 新进程启动 → 原进程退出
    └─ 用户取消 → emit errorOccurred("用户取消了权限请求")

权限影响：
  MainWindow::disableAdminFunctions()
    └─ 禁用按钮：applyButton, addButton, updateButton, deleteButton

  MainWindow::showAdminWarning()
    ├─ 弹窗询问是否提权
    ├─ 是 → requestAdminPrivileges()
    └─ 否 → disableAdminFunctions()
```

---

## 6. 数据结构定义

### 6.1 config.json 格式

**文件路径**：`<程序目录>/config/config.json`

**JSON Schema**：

```json
{
  "[以太网] 办公室网络": {
    "interface": "以太网",
    "method": "static",
    "ip": "192.168.1.100",
    "subnet": "255.255.255.0",
    "gateway": "192.168.1.1",
    "primary_dns": "8.8.8.8",
    "secondary_dns": "8.8.4.4"
  },
  "[WLAN] 自动获取": {
    "interface": "WLAN",
    "method": "dhcp",
    "ip": "",
    "subnet": "",
    "gateway": "",
    "primary_dns": "",
    "secondary_dns": ""
  }
}
```

外层为 `QJsonObject`，键为配置名称（格式 `[网卡名] 配置名`），值为配置详情对象。

### 6.2 配置 QVariantMap 字段定义

| 字段名 | 类型 | 必填 | 说明 | 有效值 / 格式 |
|--------|------|------|------|--------------|
| `interface` | `QString` | ✅ 是 | 网络接口名称 | 如 `"以太网"`、`"WLAN"` |
| `method` | `QString` | ✅ 是 | IP 获取方式 | `"static"` 或 `"dhcp"` |
| `ip` | `QString` | ⚠️ static 时必填 | IPv4 地址 | `"x.x.x.x"` 格式，每段 0–255 |
| `subnet` | `QString` | ⚠️ static 时必填 | 子网掩码 | `"x.x.x.x"` 格式 |
| `gateway` | `QString` | 否（可选） | 默认网关 | `"x.x.x.x"` 格式或空字符串 |
| `primary_dns` | `QString` | 否（可选） | 首选 DNS 服务器 | `"x.x.x.x"` 格式或空字符串 |
| `secondary_dns` | `QString` | 否（可选） | 备用 DNS 服务器 | `"x.x.x.x"` 格式或空字符串 |

**验证规则（由 ConfigManager::validateConfig 实施）**：

- `interface` 和 `method` 为必备字段，缺失则拒绝
- `method` 必须为 `"dhcp"` 或 `"static"`
- 当 `method == "static"` 时，`ip` 和 `subnet` 为必填项
- 所有 IP 格式字段（ip, subnet, gateway, primary_dns, secondary_dns）若提供值则必须通过正则 `^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$` 校验，且每段数字在 0–255 范围内

### 6.3 posConfig.ini 格式

**文件路径**：`<程序目录>/config/posConfig.ini`

通过 `QSettings` 以 INI 格式存储，典型内容：

```ini
[FloatWindow]
pos=@Point(1280 720)
```

### 6.4 窗口状态（QSettings 默认路径）

通过 `QSettings` 默认构造写入 Windows 注册表或 INI 文件，关键键值：

| 键名 | 类型 | 说明 |
|------|------|------|
| `windowGeometry` | `QByteArray` | 主窗口几何信息 |
| `windowState` | `QByteArray` | 主窗口状态（工具栏等） |
| `floatWindowVisible` | `bool` | 悬浮窗可见性 |
| `floatWindowPos` | `QPoint` | 悬浮窗位置 |

### 6.5 配置历史记录格式

**文件路径**：`<程序目录>/config/config_history.json`

存储所有配置的历史版本记录，格式为：

```json
{
  "[以太网] 办公室网络": {
    "2026-06-02T10:30:00": {
      "interface": "以太网",
      "method": "static",
      "ip": "192.168.1.100",
      "subnet": "255.255.255.0",
      "gateway": "192.168.1.1",
      "primary_dns": "8.8.8.8",
      "secondary_dns": "8.8.4.4"
    },
    "2026-06-02T11:45:00": {
      "interface": "以太网",
      "method": "static",
      "ip": "192.168.1.101",
      "subnet": "255.255.255.0",
      "gateway": "192.168.1.1",
      "primary_dns": "8.8.8.8",
      "secondary_dns": "8.8.4.4"
    }
  }
}
```

---

## 7. 新增功能模块设计

### 7.1 配置版本管理

**职责定位**：管理配置的历史版本，支持版本回滚和对比。

**新增方法（ConfigManager）**：

| 方法 | 返回值 | 说明 |
|------|--------|------|
| `getConfigHistory(configName)` | `QMap<QDateTime, QVariantMap>` | 获取指定配置的历史版本列表 |
| `rollbackConfig(configName, timestamp)` | `bool` | 回滚配置到指定版本 |
| `getConfigVersionCount(configName)` | `int` | 获取配置的版本数量 |
| `loadHistory()` | `bool` | 从文件加载历史记录 |
| `saveHistory()` | `bool` | 保存历史记录到文件 |
| `saveConfigVersion(configName, config)` | `void` | 保存配置版本到历史记录 |

**关键实现逻辑**：

- **版本存储**：每次添加或更新配置时，调用 `saveConfigVersion()` 将当前配置保存到历史记录中
- **版本限制**：每个配置最多保留 10 个历史版本，超过时自动删除最旧版本
- **历史文件**：历史记录存储在 `config_history.json` 文件中

### 7.2 性能监控

**职责定位**：记录关键操作的执行时间，提供性能统计信息。

**新增方法（Logger）**：

| 方法 | 返回值 | 说明 |
|------|--------|------|
| `startOperation(operationName)` | `void` | 记录操作开始时间 |
| `endOperation(operationName)` | `void` | 记录操作结束时间并记录日志 |
| `getOperationStats(operationName)` | `QString` | 获取操作统计信息 |
| `resetOperationStats()` | `void` | 重置所有操作统计信息 |

**统计数据结构**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `count` | `int` | 调用次数 |
| `totalTime` | `qint64` | 总耗时（毫秒） |
| `minTime` | `qint64` | 最小耗时（毫秒） |
| `maxTime` | `qint64` | 最大耗时（毫秒） |

**关键实现逻辑**：

- 使用 `QMap<QString, qint64>` 存储操作开始时间
- 使用 `QMap<QString, OperationStats>` 存储操作统计信息
- `endOperation()` 计算耗时并记录到日志系统

### 7.3 国际化支持

**职责定位**：确保所有用户可见字符串支持多语言翻译。

**实现策略**：

- 使用 Qt 的 `tr()` 函数包裹所有用户可见字符串
- 支持生成 `.ts` 翻译文件
- 当前已完成 `mainwindow.cpp` 中所有菜单字符串的国际化处理

**已国际化的字符串示例**：

| 原始字符串 | 国际化后 |
|-----------|---------|
| "显示主窗口" | `tr("显示主窗口")` |
| "显示/隐藏悬浮球" | `tr("显示/隐藏悬浮球")` |
| "启用网卡" | `tr("启用网卡")` |
| "禁用网卡" | `tr("禁用网卡")` |
| "导出日志" | `tr("导出日志")` |

## 8. v2.2.1 网络工具设计

- `ConfigManager` 在调用 `netsh` 前通过 `NetworkInterfaceManager::captureConfig()` 读取网卡现状，并用 `QSaveFile` 持久化到 `config/network_backups.json`。备份不可用时停止应用；回滚调用同一配置应用路径，成功后移除该次备份。
- `NetworkToolsDialog` 按选中网卡依次应用保存的模板或 DHCP；静态 IP 批量应用前确认冲突风险。
- `NetworkScanDialog` 将 IPv4 区间或 CIDR 展开后，以 `QtConcurrent::mapped` 扫描 ICMP/TCP；进度条按已处理地址更新，同时展示发现设备数、IP、MAC、主机名和开放端口。
- `NetworkDiagnosticsDialog` 优先选择同时具有网关和 DNS 的网卡，以工作线程执行延迟检测；下载测速使用 Windows WinHTTP/Schannel 并发下载并按时间采样，不保存下载数据。`NetworkTrafficMonitor` 读取所选网卡字节计数，计算实时速率。
- `MainWindow::setupUi()` 在主页面插入网络工具区，复用深色/浅色 QSS。配置历史随新增和修改持久化到 `config/config_history.json`，回滚后原子保存配置。

发布版由 `windeployqt` 收集 Qt/MinGW 运行时，`installer.iss` 打包为当前用户安装版；配置文件在程序首次运行时创建，安装包不包含开发机配置。
