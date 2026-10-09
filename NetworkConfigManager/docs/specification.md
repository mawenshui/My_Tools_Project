# NetworkConfigManager 项目规范文档

> **版本**：2.2.2
> **更新日期**：2026-10-09  
> **状态**：现行规范

---

## 一、文档概述

本文档定义了 NetworkConfigManager 项目的开发规范，涵盖编码风格、命名约定、文件组织、目录结构、构建发布流程以及版本管理等方面的统一标准。所有参与本项目的开发者均应遵循本文档所述规范。

NetworkConfigManager 是基于 Qt 5.12.12 + C++14 开发的 Windows 桌面网络配置管理工具，使用 qmake 构建系统，MinGW 64-bit 编译器编译。以下规范均基于项目源码中实际观察到的编写风格提炼而成，部分未在源码中体现但合理的条目会标注为“建议”。

---

## 二、编码规范

### 2.1 C++ 编码风格

#### 2.1.1 文件编码

所有源文件（`.h`、`.cpp`、`.ui`、`.qrc`、`.pro`、`.qss`）统一使用 **UTF-8** 编码。

#### 2.1.2 缩进与空格

- 缩进使用 **4 个空格**，不使用 Tab 字符。
- `public`、`protected`、`private` 访问说明符与 `class` 关键字对齐（不额外缩进）。
- 大括号风格为 **Allman 风格**（大括号另起一行）：

```cpp
if (condition)
{
    // do something
}
else
{
    // do something else
}
```

- 函数体的开大括号紧随函数签名之后，与参数列表同行：

```cpp
void MainWindow::setupUi()
{
    Logger::debug("设置UI初始状态");
    // ...
}
```

- 初始化列表中的每个成员独占一行，逗号在行尾：

```cpp
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_configManager(new ConfigManager(this)),
    m_quickMenu(nullptr),
    m_floatWindow(new FloatWindow(this)),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_floatVisible(true),
    m_autostart(false)
{
```

#### 2.1.3 头文件保护

使用传统的 `#ifndef` / `#define` / `#endif` 方式，并在 `#endif` 后添加注释标注头文件名：

```cpp
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

// ... 头文件内容 ...

#endif //CONFIGMANAGER_H
```

宏名称采用全大写文件名 + `_H` 后缀。

#### 2.1.4 注释规范

- 使用 **Doxygen 风格** 注释，支持 `@brief`、`@param`、`@return` 标签。
- 每个公开方法、信号和重要的私有方法均需添加文档注释。
- 注释语言：中英文混用，类和方法描述以中文为主，参数名和返回值用英文或中文。

```cpp
/**
 * @brief 从文件加载所有配置
 * @param useCache 是否使用缓存数据
 * @return 加载是否成功
 */
bool loadConfigs();
```

- 单行注释使用 `//` 风格，对关键逻辑加以说明。
- 头文件中按功能分组的方法区间使用 `//---` 分隔线：

```cpp
//权限检查方法组 -----------------------------------------------

//配置管理方法组 -----------------------------------------------
```

#### 2.1.5 代码分块

类的声明严格按以下顺序组织：
1. `Q_OBJECT` 宏
2. `public` 区域（构造函数、析构函数、公开方法）
3. `signals` 区域
4. `protected` 区域（事件重写等）
5. `private slots` 区域
6. `private` 区域（成员变量、私有方法）

`public` 和 `private` 区域内，功能相近的方法应分组排列，并在每组前添加 `//---` 分隔线和分组注释。

#### 2.1.6 常量定义

- 全局常量在头文件中使用 `const` 声明，放置在 `#include` 区域之后、类声明之前。
- 文件级常量在 `.cpp` 文件开头、`#include` 之后定义。
- 常量名采用驼峰式命名（如 `configFileName`、`MAIN_WINDOW_TITLE`、`APP_NAME`）。
- 字符串常量建议（源码中同时存在 `camelCase` 和 `UPPER_CASE` 两种风格，建议统一为 `UPPER_CASE`）：

```cpp
const QString MAIN_WINDOW_TITLE = "IP配置管理器(by:mws)";
const QString APP_NAME = "NetworkConfigManager";
const int interfaceCacheTimeout = 30;
```

#### 2.1.7 前向声明与 include 顺序

- 头文件中优先使用前向声明（`class Foo;`），仅在必要时 `#include` 其他头文件。
- `#include` 顺序：**自身头文件** → **Qt 库头文件** → **项目内头文件** → **系统头文件**。
- 全局 include 和常量声明放在类声明之前。

```cpp
#include "configmanager.h"       // 自身头文件
#include <QCoreApplication>      // Qt 库
#include <QDir>
#include "Logger.h"              // 项目内
#include <windows.h>             // 系统头文件
#include <sddl.h>
```

#### 2.1.8 代码行宽

单个函数不应过长。复杂功能应拆分为多个子方法，保持每个方法的单一职责。源码中 `validateConfig`、`applyConfig`、`updateQuickMenu` 等方法内部逻辑较复杂，但整体上保持了职责清晰。

---

### 2.2 Qt 特定规范

#### 2.2.1 信号与槽

- 优先使用 **新式连接语法**（基于函数指针），仅在需要函数重载时使用 `QOverload`：

```cpp
connect(ui->configList, &QListWidget::itemClicked, this, &MainWindow::onConfigSelected);
connect(ui->interfaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onInterfaceChanged);
```

- Lambda 连接用于需要内联处理的场景，捕获列表应明确指定变量：

```cpp
connect(m_configManager, &ConfigManager::configApplied, this, [this](bool success, const QString &message) {
    if (success) {
        Logger::info(tr("%1").arg(message));
    }
});
```

#### 2.2.2 信号命名

- 信号名称使用 **描述性过去式动词**：`configApplied`、`errorOccurred`、`adminStatusChanged`、`doubleClicked`。
- 信号参数使用 `const` 修饰（基础类型可不加）。

#### 2.2.3 槽命名

- 槽函数以 `on` 为前缀 + 动作描述：`onApplyConfig`、`onAddConfig`、`onDeleteConfig`、`onConfigSelected`。
- UI 自动连接的槽（由 Qt Designer 生成）使用 `on_<控件名>_<信号名>` 格式：`on_networkInterfaceCombo_currentTextChanged`。

#### 2.2.4 UI 文件

- 主窗口布局使用 Qt Designer 的 `.ui` 文件（`mainwindow.ui`），生成代码在 `ui_mainwindow.h` 中。
- UI 对象通过 `ui` 指针访问（`ui->setupUi(this)` 后初始化）。
- 用户不应手动修改 `ui_mainwindow.h`。

#### 2.2.5 资源管理

- 所有图标、图片和样式表通过 `.qrc` 资源文件管理。
- 资源前缀按类型分组：`/images` 用于图标，`/styles` 用于样式表。
- 资源引用使用 `qrc` 路径格式：`:/images/images/icon.png`、`:/styles/styles/drak_theme.qss`。

```xml
<qresource prefix="/images">
    <file>images/icon.ico</file>
    <file>images/icon.png</file>
</qresource>
<qresource prefix="/styles">
    <file>styles/drak_theme.qss</file>
</qresource>
```

#### 2.2.6 样式表

- 样式表文件（`.qss`）放置在 `styles/` 目录下。
- 通过 `QFile` 读取后调用 `qApp->setStyleSheet()` 或 `widget->setStyleSheet()` 应用。
- 样式表文件在 `.qrc` 中注册，通过资源路径加载。

#### 2.2.7 Q_OBJECT 宏

所有继承自 `QObject`（及其子类）的类必须包含 `Q_OBJECT` 宏。该宏紧跟在类声明的开大括号之后：

```cpp
class ConfigManager : public QObject
{
    Q_OBJECT
```

#### 2.2.8 国际化

使用 `tr()` 函数包裹用户可见的字符串，为未来的国际化支持做好准备。

---

## 三、命名约定

### 3.1 类命名

- 使用 **PascalCase**（大驼峰）命名法。
- 类名应为名词或名词短语，准确描述其职责。

| 类名 | 职责 |
|---|---|
| `MainWindow` | 主窗口，负责 UI 交互和整体控制 |
| `ConfigManager` | 配置管理，负责配置的增删改查与应用 |
| `NetworkInterfaceManager` | 网络接口管理，负责网卡列表获取和启禁操作 |
| `FloatWindow` | 悬浮窗口，提供快捷操作入口 |
| `Logger` | 日志记录器，提供线程安全的日志功能 |

### 3.2 方法命名

- 使用 **camelCase**（小驼峰）命名法。
- 方法名应为动词或动词短语，清晰表达其操作。

| 方法名 | 说明 |
|---|---|
| `loadConfigs()` | 加载配置 |
| `applyConfig()` | 应用配置 |
| `addConfig()` | 添加配置 |
| `validateConfig()` | 验证配置 |
| `setupUi()` | 初始化 UI |
| `setupConnections()` | 建立信号槽连接 |
| `checkAdminStatus()` | 检查管理员状态 |
| `requestAdminPrivileges()` | 请求管理员权限 |
| `getNetworkInterfaces()` | 获取网络接口列表 |

### 3.3 成员变量命名

- 非静态成员变量以 **`m_` 前缀** + camelCase 命名。
- 布尔变量以 `is` 或 `has` 开头表达状态。

| 成员变量 | 说明 |
|---|---|
| `m_configFile` | 配置文件路径 |
| `m_isAdmin` | 管理员状态标志 |
| `m_dragPosition` | 拖动起始位置 |
| `m_backgroundColor` | 背景颜色 |
| `m_hasBackgroundPixmap` | 是否有背景图标 |

### 3.4 信号命名

- 使用描述性 **过去式动词** 或 **已完成动作** 的表述。
- 如 `configApplied`（配置已应用）、`errorOccurred`（错误已发生）、`adminStatusChanged`（管理员状态已变化）。

### 3.5 槽命名

- 使用 **`on` + 动作** 格式：`onAddConfig`、`onDeleteConfig`、`onApplyConfig`。
- 自动连接的 UI 槽使用 `on_<控件名>_<信号名>` 格式。

### 3.6 文件命名

- 文件名与其中主要类的类名保持一致。
- 头文件扩展名：`.h`
- 源文件扩展名：`.cpp`
- 资源文件扩展名：`.qrc`
- 项目文件扩展名：`.pro`
- UI 文件扩展名：`.ui`

| 文件 | 说明 |
|---|---|
| `configmanager.h` / `configmanager.cpp` | ConfigManager 类 |
| `mainwindow.h` / `mainwindow.cpp` | MainWindow 类 |
| `floatwindow.h` / `floatwindow.cpp` | FloatWindow 类 |
| `logger.h` | Logger 类（仅头文件） |
| `networkinterfacemanager.h` / `networkinterfacemanager.cpp` | NetworkInterfaceManager 类 |

> **注意**：`Logger.h` 采用类的 PascalCase 命名，与其他头文件一致，无独立的 `.cpp` 文件（所有实现均在头文件中内联）。

### 3.7 局部变量与参数命名

- 局部变量和函数参数使用 **camelCase**。
- 参数名应具有描述性：`useCache`、`oldName`、`newName`、`InterfaceName`（源码中同时存在 `interfaceName` 和 `InterfaceName` 两种风格，建议统一为 `camelCase`）。

---

## 四、文件组织规范

### 4.1 头文件结构

头文件应按下述顺序组织内容：

```
1. 头文件保护宏 (#ifndef / #define)
2. #include 区域（Qt 库 → 项目内 → 系统）
3. 全局常量定义（如适用）
4. 前向声明 / namespace 前置声明
5. 类 Doxygen 注释
6. 类声明
   a. Q_OBJECT 宏
   b. public 区域（构造/析构 + 公开方法，按功能分组）
   c. signals 区域
   d. protected 区域（事件重写等）
   e. private slots 区域
   f. private 区域（成员变量 + 私有方法，按功能分组）
7. 头文件保护结束宏 (#endif)
```

参考文件：[configmanager.h](../configmanager.h)、[floatwindow.h](../floatwindow.h)

### 4.2 源文件结构

源文件应按下述顺序组织内容：

```
1. #include "自身头文件"
2. 其他 #include（Qt 库 → 项目内 → 系统）
3. 文件级常量定义
4. 构造函数实现
5. 析构函数实现
6. 公开方法实现（按 .h 中的声明顺序）
7. 私有方法实现（按 .h 中的声明顺序）
```

参考文件：[configmanager.cpp](../configmanager.cpp)、[floatwindow.cpp](../floatwindow.cpp)

### 4.3 分段规则

- 头文件中，`public`、`signals`、`protected`、`private slots`、`private` 各区域之间用空行分开。
- 同一区域内的方法按功能分组，组间使用带分隔线的注释：

```cpp
//配置管理方法组 -----------------------------------------------

//工具方法组 --------------------------------------------------
```

---

## 五、目录结构规范

### 5.1 目录总览

```
NetworkConfigManager/
├── bin/                           # 构建输出目录
│   ├── config/                    # 运行时配置文件目录
│   │   ├── config.json            # 网络配置数据文件
│   │   └── posConfig.ini         # 窗口位置等持久化配置
│   ├── logs/                      # 日志文件目录
│   │   └── app_YYYYMMDD.log       # 按日期命名的日志文件
│   └── NetworkConfigManager.exe   # 可执行文件
├── images/                        # 图标和图片资源目录
│   ├── icon.ico                   # 应用程序图标
│   ├── icon.png                   # 默认状态图标
│   ├── icon_green.png             # 成功状态图标
│   ├── icon_red.png               # 失败状态图标
│   ├── float_icon.png             # 悬浮窗图标
│   ├── indicator_green.png        # 绿色状态指示灯
│   └── indicator_red.png          # 红色状态指示灯
├── styles/                        # QSS 样式表目录
│   └── drak_theme.qss             # 暗色主题样式表
├── docs/                          # 项目文档目录
│   └── specification.md           # 本文档（项目规范文档）
├── main.cpp                       # 程序入口
├── mainwindow.h / mainwindow.cpp  # 主窗口类
├── mainwindow.ui                  # Qt Designer UI 文件
├── configmanager.h / configmanager.cpp  # 配置管理类
├── floatwindow.h / floatwindow.cpp      # 浮动窗口类
├── networkinterfacemanager.h / networkinterfacemanager.cpp  # 网卡管理类
├── logger.h                       # 日志记录器类
├── resources.qrc                  # Qt 资源文件
├── NetworkConfigManager.pro       # qmake 项目文件
└── README.md                      # 项目说明文档
```

### 5.2 各目录职责

| 目录 | 职责 | 规则 |
|---|---|---|
| `bin/` | 构建产物输出目录 | 由 qmake 的 `DESTDIR` 配置自动生成，不应手动提交其中内容到版本控制 |
| `bin/config/` | 运行时配置文件 | 由程序自动创建和管理，不应删除 |
| `bin/logs/` | 运行时日志文件 | 日志按日期滚动，应定期清理过期日志 |
| `images/` | 图标和图片资源 | 仅存放 `.png`、`.ico` 等静态资源文件 |
| `styles/` | QSS 样式表 | 仅存放 `.qss` 文件，供主题切换使用 |
| `docs/` | 项目文档 | 存放规范文档、设计文档等 Markdown 文件 |
| 项目根目录 | 源代码和构建文件 | 仅放置 `.h`、`.cpp`、`.ui`、`.qrc`、`.pro` 及 `README` |

### 5.3 文件放置规则

- **新增源文件**：`.h` 和 `.cpp` 文件放置在项目根目录下，并在 `.pro` 文件的 `HEADERS` / `SOURCES` 中注册。
- **新增 UI 文件**：`.ui` 文件放置在项目根目录下，并在 `.pro` 文件的 `FORMS` 中注册。
- **新增资源**：图片放入 `images/` 目录，样式表放入 `styles/` 目录，并在 `resources.qrc` 中注册对应的文件路径。
- **新增文档**：放入 `docs/` 目录。
- **构建产物**：不应提交到版本控制，应在 `.gitignore` 中忽略 `bin/` 目录。

---

## 六、构建与发布流程

### 6.1 环境要求

| 项目 | 版本/说明 |
|---|---|
| 操作系统 | Windows 10 / 11 (64-bit) |
| 编译器 | MinGW-w64 64-bit |
| Qt 版本 | 5.12.12 |
| 构建系统 | qmake (Qt 自带) |
| C++ 标准 | C++14 |
| 关键依赖 | Qt Core, Qt GUI, Qt Network, Qt Widgets |

### 6.2 编译步骤

**1. 环境准备**

确保 Qt 5.12.12 (MinGW 64-bit) 已正确安装，并将 `bin` 目录添加到系统 `PATH` 环境变量中。

**2. 生成 Makefile**

```bash
cd NetworkConfigManager
qmake NetworkConfigManager.pro
```

**3. 编译项目**

```bash
mingw32-make
```

或使用 Qt Creator 打开 `NetworkConfigManager.pro` 后点击构建。

**4. 产物位置**

编译产物（`NetworkConfigManager.exe`）输出至 `bin/` 目录。

### 6.3 构建配置说明

项目文件 [NetworkConfigManager.pro](../NetworkConfigManager.pro) 中的关键配置：

| 配置项 | 值 | 说明 |
|---|---|---|
| `QT` | `core gui network concurrent widgets` | 所需 Qt 模块 |
| `CONFIG` | `c++14` | C++14 标准 |
| `TARGET` | `NetworkConfigManager` | 可执行文件名 |
| `TEMPLATE` | `app` | 应用程序模板 |
| `DESTDIR` | `$$PWD/bin` | 输出目录 |
| `QMAKE_CXXFLAGS` | `-Wall -Wextra -Werror=return-type` | 编译警告选项 |
| `DEFINES` | `QT_DEPRECATED_WARNINGS` | 启用废弃 API 警告 |
| `LIBS` | `-lDbgHelp -lIphlpapi -lWs2_32` | 链接转储与网络 API |

### 6.4 发布产物

发布包应包含以下文件：

| 文件 | 必需 | 说明 |
|---|---|---|
| `NetworkConfigManager.exe` | 是 | 主程序 |
| 依赖的 Qt DLL 文件 | 是 | 使用 `windeployqt` 工具自动收集 |
| `bin/config/` 目录 | 否 | 程序运行后自动创建 |
| `bin/logs/` 目录 | 否 | 程序运行后自动创建 |

使用 `windeployqt` 部署 Qt 依赖：

```bash
cd bin
windeployqt NetworkConfigManager.exe
```

---

## 七、Git 提交规范

### 7.1 提交信息格式（建议）

提交信息建议采用 Conventional Commits（约定式提交）格式（译注：源码注释和提交风格体现了干净、简洁的原则，本条目为基于行业最佳实践的建议）：

```
<类型>(<范围>): <简短描述>

[可选的详细描述]

[可选的脚注]
```

**类型（type）**：
| 类型 | 说明 |
|---|---|
| `feat` | 新功能 |
| `fix` | Bug 修复 |
| `refactor` | 代码重构（不改变功能） |
| `style` | 代码风格调整（不影响功能） |
| `docs` | 文档变更 |
| `chore` | 构建或辅助工具变更 |
| `perf` | 性能优化 |

**示例**：

```
feat(config): 添加配置导入导出功能

支持将网络配置导出为 JSON 文件，或从 JSON 文件导入配置。

Closes #42
```

```
fix(ui): 修复悬浮窗在多显示器环境下的位置异常

在高 DPI 显示器上悬浮窗位置计算不正确，已修正坐标转换逻辑。
```

### 7.2 分支管理（建议）

建议采用简化版 Git Flow 分支策略：

| 分支 | 用途 |
|---|---|
| `main` | 稳定版本，仅通过 PR 合并 |
| `develop` | 开发主线 |
| `feature/<功能名>` | 功能开发分支，从 `develop` 拉出 |
| `fix/<问题描述>` | Bug 修复分支，从 `develop` 或 `main` 拉出 |

### 7.3 提交前检查

- 确保代码通过编译（`mingw32-make` 无错误）。
- 确保无编译警告（`-Wall -Wextra` 下零警告）。
- 确保新代码符合本文档中的编码规范。
- 建议：提交前先在本地执行 `qmake && mingw32-make clean && mingw32-make` 验证构建。

---

## 八、版本号管理规范

### 8.1 版本号格式

采用 **语义化版本号**（Semantic Versioning 2.0.0）格式：

```
<主版本号>.<次版本号>.<修订号>
MAJOR.MINOR.PATCH
```

**当前版本**：**2.2.2**

### 8.2 版本号递增规则

| 段位 | 递增条件 | 示例 |
|---|---|---|
| 主版本号 (MAJOR) | 不兼容的 API 修改、重大架构变更 | 2.1.0 → 3.0.0 |
| 次版本号 (MINOR) | 向下兼容的新功能、UI 改进 | 2.1.0 → 2.2.0 |
| 修订号 (PATCH) | 向下兼容的 Bug 修复 | 2.1.0 → 2.1.1 |

### 8.3 版本号定义位置

版本号在 [main.cpp](../main.cpp) 中定义：

```cpp
a.setApplicationVersion("2.2.2");
```

发布新版本时，应同步更新此处的版本号字符串。

---

> **文档维护说明**：本文档应随着项目的发展持续更新。当编码规范、目录结构或构建流程发生变更时，请同步修改本文档。对于源码中暂时未体现但希望在后续项目中采用的规范，建议通过团队讨论后明确加入本文档。

## 九、v2.2.2 发布检查

1. 在 Qt 5.12.12 MinGW 64 位环境以 Release 配置构建，保留隔离目录 `build_auto/app/bin/NetworkConfigManager.exe`。
2. 运行 `tests/configmanager_test.pro`、`tests/networkscan_selfcheck.pro`、`tests/ui_selfcheck.pro` 的可执行检查，设置 `QT_QPA_PLATFORM=offscreen`；在可访问公网环境设置 `NETWORKCONFIGMANAGER_TEST_HTTPS=1` 并用 `NETWORKCONFIGMANAGER_TEST_URL` 分别验证 Cloudflare 和 Hetzner 预设。网络修改类操作在隔离管理员环境单独验收。
3. 执行 `package.ps1 -QtBin <Qt bin目录> -Iscc <Inno Setup 6 ISCC.exe>`；脚本收集运行库、生成安装包和免安装 ZIP，输出 SHA-256。
4. 安装版默认放在当前用户目录，并提供卸载；不打包 `bin/config/`、日志或开发机历史。校验安装后可启动、卸载后用户数据处理符合预期。
5. GitHub 发布使用 `networkconfigmanager-v2.2.2` 标签，上传安装包和 ZIP；代码、文档、测试与打包脚本保持同一提交。
