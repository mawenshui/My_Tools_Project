# 文件转移工具 (FileTransferTool)

[![Version](https://img.shields.io/badge/version-3.2.0-blue.svg)](https://github.com/your-repo/FileTransferTool)
[![Qt](https://img.shields.io/badge/Qt-5.12+-green.svg)](https://www.qt.io/)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)
[![License](https://img.shields.io/badge/license-Personal%20Use-orange.svg)](#许可证)

## 📋 目录

- [项目简介](#项目简介)
- [功能特性](#功能特性)
- [系统要求](#系统要求)
- [快速开始](#快速开始)
- [使用方法](#使用方法)
- [项目结构](#项目结构)
- [技术架构](#技术架构)
- [开发指南](#开发指南)
- [API文档](#api文档)
- [常见问题](#常见问题)
- [版本历史](#版本历史)
- [贡献指南](#贡献指南)
- [许可证](#许可证)

## 📖 项目简介

文件转移工具是一个基于Qt开发的现代化桌面应用程序，专为高效的文件和文件夹批量管理而设计。该工具集成了文件传输、文件管理、智能筛选等多项功能，提供直观的图形界面和强大的文件处理能力。

### 🎯 设计目标
- **高效性**: 多线程处理，支持大批量文件操作
- **智能化**: 多维度文件筛选，智能冲突处理
- **易用性**: 直观的用户界面，丰富的操作反馈
- **可靠性**: 完善的错误处理，安全的文件操作
- **扩展性**: 模块化设计，便于功能扩展

## ✨ 功能特性

### 🚀 文件传输功能

#### 核心传输能力
- **📁 多源文件夹选择**: 支持同时选择多个源文件夹进行批量处理
- **🎯 目标目录指定**: 灵活选择文件转移的目标位置
- **⚡ 多线程处理**: 异步文件操作，不阻塞用户界面
- **📊 实时进度显示**: 精确的进度条和处理状态反馈
- **📝 操作日志**: 详细记录转移过程和结果

#### 转移模式
- **🏗️ 保持结构模式**: 完整保留原有文件夹层次结构
- **📄 扁平化模式**: 将所有文件提取到单一目录层级

#### 智能文件筛选系统
- **🎨 文件类型筛选**: 
  - 图片文件 (jpg, png, gif, bmp, svg 等)
  - 文档文件 (doc, pdf, txt, xlsx 等)
  - 视频文件 (mp4, avi, mkv, mov 等)
  - 音频文件 (mp3, wav, flac, aac 等)
  - 压缩文件 (zip, rar, 7z, tar 等)
  - 可执行文件 (exe, msi, bat 等)
- **📏 文件大小筛选**: 设置最小/最大文件大小限制
- **⏰ 修改时间筛选**: 根据文件修改时间范围进行筛选
- **🔍 文件名模式匹配**: 
  - 通配符匹配 (*.txt, image_*.jpg)
  - 正则表达式匹配
- **🚫 排除列表**: 指定要排除的文件或文件夹名称
- **🔄 文件冲突处理**: 支持覆盖现有文件或自动重命名

### 🗂️ 文件管理功能

#### 文件浏览器
- **📂 直观界面**: 现代化的文件和文件夹浏览界面
- **🧭 智能导航**: 
  - 磁盘驱动器选择下拉框
  - 前进、后退、上级目录按钮
  - 当前路径显示和编辑
- **📋 文件列表**: 详细的文件信息显示（名称、大小、修改时间）

#### 文件操作
- **📋 复制/剪切/粘贴**: 完整的剪贴板操作支持
- **🗑️ 删除**: 安全的文件和文件夹删除
- **✏️ 重命名**: 就地重命名文件和文件夹
- **🔍 文件查找**: 基于文件名的快速搜索
- **ℹ️ 属性查看**: 查看文件详细信息（大小、修改时间、权限等）
- **🖱️ 右键菜单**: 便捷的上下文操作菜单

### 🎨 用户界面
- **🌟 现代化设计**: 简洁美观的Material Design风格界面
- **📑 选项卡布局**: 文件传输和文件管理功能模块化分离
- **⚡ 实时反馈**: 操作状态和进度的实时显示
- **🚨 智能提示**: 友好的错误信息和处理建议
- **🎯 响应式布局**: 适配不同窗口大小的界面布局

## 💻 系统要求

### 最低系统要求
- **操作系统**: Windows 7 SP1 / 8.1 / 10 / 11 (64位推荐)
- **处理器**: Intel Core i3 或 AMD 同等级别处理器
- **内存**: 4GB RAM (推荐 8GB 以上)
- **磁盘空间**: 至少 200MB 可用空间
- **显示器**: 1024x768 分辨率 (推荐 1920x1080)

### 开发环境要求
- **Qt版本**: Qt 5.12.0 或更高版本 (推荐 Qt 5.15.2)
- **编译器**: 
  - MinGW 7.3.0+ (推荐)
  - MSVC 2017+ (可选)
  - GCC 7.0+ (Linux)
- **构建工具**: qmake 或 CMake
- **IDE**: Qt Creator 4.8+ (推荐) 或 Visual Studio 2017+

## 🚀 快速开始

### 方式一：下载预编译版本

1. 前往 [Releases](https://github.com/your-repo/FileTransferTool/releases) 页面
2. 下载最新版本的 `FileTransferTool-v3.2.0-windows.zip`
3. 解压到任意目录
4. 双击 `FileTransferTool.exe` 运行

### 方式二：从源码编译

#### 使用 Qt Creator (推荐)

1. **安装 Qt 开发环境**
   ```bash
   # 下载 Qt 在线安装器
   # https://www.qt.io/download-qt-installer
   ```

2. **获取源代码**
   ```bash
   git clone https://github.com/your-repo/FileTransferTool.git
   cd FileTransferTool
   ```

3. **打开项目**
   - 启动 Qt Creator
   - 打开 `FileTransferTool.pro` 文件
   - 选择合适的构建套件 (MinGW 64-bit)

4. **编译运行**
   - 点击 "构建" 按钮 (Ctrl+B)
   - 点击 "运行" 按钮 (Ctrl+R)

#### 使用命令行编译

```bash
# 确保 Qt 环境变量已设置
# 生成 Makefile
qmake FileTransferTool.pro

# 编译项目 (Windows)
mingw32-make

# 或者使用 nmake (MSVC)
# nmake

# 运行程序
cd release
.\FileTransferTool.exe
```

#### 使用 CMake 编译 (可选)

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## 📖 使用方法

### 🚀 文件传输功能

#### 基本传输流程

1. **📁 选择源文件夹**
   ```
   点击 "添加源文件夹" → 选择文件夹 → 确认添加
   ```
   - 支持添加多个源文件夹
   - 可以通过列表管理已添加的文件夹
   - 支持删除不需要的源文件夹

2. **🎯 选择目标目录**
   ```
   点击 "选择目标目录" → 浏览并选择目标位置 → 确认
   ```
   - 目标目录会显示在界面上
   - 支持创建新文件夹作为目标

3. **⚙️ 配置传输选项**
   
   **传输模式选择：**
   - 🏗️ **保持结构模式**: 完整保留源文件夹的目录结构
     ```
     源: folder1/subfolder/file.txt
     目标: target/folder1/subfolder/file.txt
     ```
   - 📄 **扁平化模式**: 将所有文件提取到目标目录根级别
     ```
     源: folder1/subfolder/file.txt
     目标: target/file.txt
     ```
   
   **文件冲突处理：**
   - ✅ **覆盖现有文件**: 直接替换同名文件
   - 🔄 **自动重命名**: 为冲突文件添加序号后缀

4. **🔍 设置智能筛选** (可选)
   
   **文件类型筛选：**
   ```
   选择文件类型 → 查看包含的扩展名 → 启用筛选
   ```
   - 图片: jpg, png, gif, bmp, svg, webp
   - 文档: doc, docx, pdf, txt, xlsx, ppt
   - 视频: mp4, avi, mkv, mov, wmv, flv
   - 音频: mp3, wav, flac, aac, ogg, m4a
   - 压缩: zip, rar, 7z, tar, gz, bz2
   - 可执行: exe, msi, bat, cmd, com
   
   **文件大小筛选：**
   ```
   设置最小值 → 设置最大值 → 选择单位(B/KB/MB/GB)
   ```
   
   **修改时间筛选：**
   ```
   设置开始时间 → 设置结束时间 → 启用时间筛选
   ```
   
   **文件名模式匹配：**
   ```
   输入模式 → 选择匹配类型(通配符/正则) → 启用筛选
   ```
   - 通配符示例: `*.txt`, `image_*.jpg`, `report_2023*`
   - 正则表达式示例: `^[A-Z].*\.pdf$`, `\d{4}-\d{2}-\d{2}.*`
   
   **排除列表：**
   ```
   输入排除项 → 每行一个 → 启用排除筛选
   ```
   - 支持文件名和文件夹名
   - 支持通配符模式

5. **▶️ 开始传输**
   ```
   检查配置 → 点击"开始传输" → 监控进度 → 查看结果
   ```
   - 实时进度条显示
   - 详细的状态信息
   - 传输完成后的结果报告

### 🗂️ 文件管理功能

#### 文件浏览操作

1. **🧭 目录导航**
   ```
   选择驱动器 → 点击文件夹 → 使用导航按钮
   ```
   - 💿 **驱动器选择**: 下拉框选择不同磁盘
   - ⬆️ **上级目录**: 返回父文件夹
   - 📍 **路径编辑**: 直接输入目标路径
   - 🔄 **刷新**: 更新当前目录内容

2. **📋 文件选择**
   - **单选**: 点击文件/文件夹
   - **多选**: Ctrl + 点击多个项目
   - **范围选择**: Shift + 点击选择范围
   - **全选**: Ctrl + A

#### 文件操作详解

1. **📋 复制/剪切/粘贴**
   ```
   选择文件 → 复制(Ctrl+C)/剪切(Ctrl+X) → 导航到目标 → 粘贴(Ctrl+V)
   ```
   - 支持跨目录操作
   - 自动处理文件名冲突
   - 显示操作进度

2. **🗑️ 删除操作**
   ```
   选择文件/文件夹 → 点击删除按钮/按Delete键 → 确认删除
   ```
   - 安全删除确认
   - 支持批量删除
   - 错误处理和提示

3. **✏️ 重命名操作**
   ```
   选择单个文件/文件夹 → 点击重命名/按F2 → 输入新名称 → 确认
   ```
   - 就地编辑模式
   - 名称合法性检查
   - 重名冲突检测

4. **🔍 文件查找**
   ```
   点击查找按钮 → 输入搜索关键词 → 查看结果列表
   ```
   - 支持通配符搜索
   - 实时搜索结果
   - 双击结果定位文件

5. **ℹ️ 属性查看**
   ```
   选择文件 → 点击属性按钮 → 查看详细信息
   ```
   - 文件大小和类型
   - 创建和修改时间
   - 文件权限信息

6. **🖱️ 右键菜单**
   ```
   右键点击文件/文件夹 → 选择操作
   ```
   - 快速访问常用操作
   - 上下文相关的菜单项
   - 键盘快捷键提示

### 详细操作说明
1. **移除文件夹**: 选中列表中的文件夹，点击"移除选中"按钮
2. **选择转移模式**:
   - **保持文件夹结构**: 维持原有的目录层次结构
   - **只提取文件(扁平化)**: 将所有文件提取到目标目录根目录，忽略原有文件夹结构
3. **配置筛选选项**（可选）:
   - **文件类型筛选**: 勾选"启用文件类型筛选"，选择预定义类型或输入自定义扩展名
     - 预定义类型会自动显示包含的后缀名，如图片文件包含：.jpg, .jpeg, .png, .gif, .bmp, .tiff, .webp, .svg, .ico
   - **文件大小筛选**: 设置最小和最大文件大小范围
   - **修改时间筛选**: 选择时间范围或自定义起止时间
   - **文件名筛选**: 使用通配符或正则表达式匹配文件名
   - **排除列表**: 输入要排除的文件或文件夹名称
4. **设置覆盖选项**: 勾选"覆盖已存在的文件"来决定如何处理同名文件
   - 勾选: 直接覆盖已存在的文件
   - 不勾选: 自动重命名文件(如 file.txt → file(1).txt)
5. **查看进度**: 转移过程中可以实时查看进度条和状态信息

## 📁 项目结构

```
FileTransferTool/
├── 📄 main.cpp                    # 应用程序入口点
├── 🏠 mainwindow.h                # 主窗口类声明
├── 🏠 mainwindow.cpp              # 主窗口类实现
├── ⚙️ filetransferworker.h        # 文件传输工作线程声明
├── ⚙️ filetransferworker.cpp      # 文件传输工作线程实现
├── 🔧 FileTransferTool.pro        # Qt qmake 项目配置文件
├── 📚 README.md                   # 项目文档 (本文件)
├── 📋 CMakeLists.txt              # CMake 构建配置 (可选)
├── 📦 release/                    # 发布版本输出目录
│   └── 🚀 FileTransferTool.exe    # 编译后的可执行文件
├── 🔍 debug/                      # 调试版本输出目录
├── 🎨 resources/                  # 资源文件目录
│   ├── 🖼️ icons/                  # 图标文件
│   └── 🎨 styles/                 # 样式表文件
└── 📖 docs/                       # 文档目录
    ├── 🏗️ architecture.md         # 架构设计文档
    ├── 🔧 api.md                  # API 参考文档
    └── 📝 changelog.md            # 版本更新日志
```

### 📋 核心文件说明

| 文件 | 功能描述 | 主要职责 |
|------|----------|----------|
| `main.cpp` | 程序入口 | 初始化Qt应用程序，创建主窗口 |
| `mainwindow.h/cpp` | 主窗口类 | UI界面管理，用户交互处理，线程调度 |
| `filetransferworker.h/cpp` | 工作线程类 | 文件传输逻辑，筛选算法，进度反馈 |
| `FileTransferTool.pro` | 项目配置 | 编译设置，依赖管理，构建规则 |

## 🏗️ 技术架构

### 🎯 架构设计原则

- **🔄 单一职责**: 每个类专注于特定功能领域
- **🔗 松耦合**: 模块间通过信号槽机制通信
- **📈 可扩展**: 插件化设计，便于功能扩展
- **🛡️ 健壮性**: 完善的错误处理和异常恢复
- **⚡ 高性能**: 多线程处理，异步操作

### 🏛️ 系统架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        用户界面层 (UI Layer)                    │
├─────────────────────────────────────────────────────────────┤
│  MainWindow                                                 │
│  ├── 文件传输界面 (File Transfer UI)                          │
│  │   ├── 源文件夹选择                                          │
│  │   ├── 目标目录选择                                          │
│  │   ├── 筛选选项配置                                          │
│  │   └── 进度显示                                            │
│  └── 文件管理界面 (File Management UI)                        │
│      ├── 文件浏览器                                           │
│      ├── 操作按钮                                            │
│      └── 右键菜单                                            │
└─────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│                       业务逻辑层 (Logic Layer)                  │
├─────────────────────────────────────────────────────────────┤
│  FileTransferWorker (工作线程)                               │
│  ├── 文件传输算法                                            │
│  ├── 智能筛选引擎                                            │
│  ├── 冲突处理机制                                            │
│  └── 进度计算                                               │
│                                                             │
│  FileManager (文件管理器)                                    │
│  ├── 文件操作 (复制/移动/删除)                                │
│  ├── 目录遍历                                               │
│  ├── 文件搜索                                               │
│  └── 属性查询                                               │
└─────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────┐
│                       数据访问层 (Data Layer)                   │
├─────────────────────────────────────────────────────────────┤
│  Qt File System API                                        │
│  ├── QFile (文件操作)                                        │
│  ├── QDir (目录操作)                                         │
│  ├── QFileInfo (文件信息)                                    │
│  └── QStandardPaths (系统路径)                               │
└─────────────────────────────────────────────────────────────┘
```

### 🔄 核心设计模式

#### 1. **观察者模式 (Observer Pattern)**
```cpp
// 信号槽机制实现观察者模式
connect(worker, &FileTransferWorker::progressUpdated, 
        this, &MainWindow::updateProgress);
```

#### 2. **策略模式 (Strategy Pattern)**
```cpp
// 不同传输模式的策略实现
enum class TransferMode {
    KeepStructure,    // 保持结构策略
    FlattenFiles      // 扁平化策略
};
```

#### 3. **工厂模式 (Factory Pattern)**
```cpp
// 文件筛选器工厂
class FilterFactory {
    static std::unique_ptr<FileFilter> createFilter(FilterType type);
};
```

### ⚡ 核心功能实现

#### 🔄 文件传输引擎
- **递归目录遍历**: 深度优先搜索算法，高效遍历文件夹结构
- **多模式文件转移**: 
  - 🏗️ **结构保持模式**: 完整复制目录树结构
  - 📄 **扁平化模式**: 递归提取所有文件到单一目录
- **异步处理**: 使用 `QThread` 实现非阻塞文件操作
- **内存优化**: 流式处理大文件，避免内存溢出

#### 🔍 智能筛选系统
- **多维度筛选**: 支持文件类型、大小、时间、名称模式和排除列表
- **预定义文件类型**: 内置常用文件类型分类及其扩展名映射
- **动态界面提示**: 实时显示选中文件类型包含的后缀名
- **灵活的匹配模式**: 支持通配符和正则表达式
- **性能优化**: 预编译正则表达式，提高匹配效率

#### 🛡️ 安全与可靠性
- **智能文件冲突处理**: 支持覆盖或自动重命名策略
- **原子操作**: 确保文件操作的完整性
- **错误恢复**: 支持操作失败后的回滚机制
- **权限检查**: 预先验证文件访问权限

#### 📊 进度与反馈
- **精确进度计算**: 预先统计总文件数和大小
- **实时状态更新**: 毫秒级进度反馈
- **详细日志记录**: 完整的操作历史和错误信息

### 🏆 代码质量
- **📝 完整注释**: 所有函数都有详细的文档注释
- **🚨 错误处理**: 全面的异常处理和用户友好的错误提示
- **🧠 内存管理**: 正确的Qt对象生命周期管理
- **🔧 模块化设计**: 功能模块清晰分离，易于维护和扩展
- **✅ 单元测试**: 核心功能的完整测试覆盖
- **📏 代码规范**: 遵循Qt和C++最佳实践

## 🛠️ 开发指南

### 🏗️ 开发环境搭建

#### 1. **安装 Qt 开发环境**
```bash
# 下载 Qt 在线安装器
# https://www.qt.io/download-qt-installer

# 推荐安装组件:
# - Qt 5.15.2 (MinGW 64-bit)
# - Qt Creator 4.15+
# - MinGW 7.3.0 64-bit
# - CMake 3.19+
```

#### 2. **配置开发环境**
```bash
# 设置环境变量 (Windows)
set QTDIR=C:\Qt\5.15.2\mingw81_64
set PATH=%QTDIR%\bin;%PATH%

# 验证安装
qmake --version
mingw32-make --version
```

#### 3. **克隆项目**
```bash
git clone https://github.com/your-repo/FileTransferTool.git
cd FileTransferTool
```

### 🔧 构建配置

#### qmake 配置 (FileTransferTool.pro)
```pro
QT += core widgets
CONFIG += c++11

TARGET = FileTransferTool
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    filetransferworker.cpp

HEADERS += \
    mainwindow.h \
    filetransferworker.h

# 发布配置
RELEASE {
    DEFINES += QT_NO_DEBUG_OUTPUT
    QMAKE_CXXFLAGS_RELEASE += -O2
}

# 调试配置
DEBUG {
    DEFINES += DEBUG_MODE
    QMAKE_CXXFLAGS_DEBUG += -g
}
```

### 📝 编码规范

#### 1. **命名约定**
```cpp
// 类名: PascalCase
class FileTransferWorker;

// 函数名: camelCase
void startTransfer();
void updateProgress();

// 变量名: camelCase
int fileCount;
QString currentPath;

// 成员变量: m_ 前缀
QString m_currentDirectory;
QProgressBar* m_progressBar;

// 常量: UPPER_CASE
const int MAX_FILE_SIZE = 1024 * 1024 * 1024;
```

#### 2. **代码风格**
```cpp
// 函数注释格式
/**
 * @brief 启动文件传输过程
 * @param sourceDir 源目录路径
 * @param targetDir 目标目录路径
 * @param options 传输选项
 * @return 是否成功启动
 */
bool startTransfer(const QString& sourceDir, 
                   const QString& targetDir, 
                   const TransferOptions& options);

// 错误处理
if (!QDir(sourceDir).exists()) {
    emit errorOccurred(tr("源目录不存在: %1").arg(sourceDir));
    return false;
}
```

### 🧪 测试指南

#### 单元测试示例
```cpp
#include <QtTest/QtTest>
#include "filetransferworker.h"

class TestFileTransferWorker : public QObject {
    Q_OBJECT

private slots:
    void testFileFiltering();
    void testProgressCalculation();
    void testConflictResolution();
};

void TestFileTransferWorker::testFileFiltering() {
    FilterOptions options;
    options.enableFileTypeFilter = true;
    options.allowedExtensions = {"txt", "pdf"};
    
    FileTransferWorker worker;
    QVERIFY(worker.passesFilter("document.pdf", options));
    QVERIFY(!worker.passesFilter("image.jpg", options));
}
```

### 🔌 扩展开发

#### 添加新的文件筛选器
```cpp
// 1. 在 FilterOptions 中添加新选项
struct FilterOptions {
    // ... 现有选项
    bool enableCustomFilter = false;
    QString customFilterScript;
};

// 2. 在 FileTransferWorker 中实现筛选逻辑
bool FileTransferWorker::passesCustomFilter(const QString& filePath, 
                                           const FilterOptions& options) {
    if (!options.enableCustomFilter) return true;
    
    // 实现自定义筛选逻辑
    // ...
}

// 3. 在 UI 中添加配置界面
void MainWindow::setupCustomFilterUI() {
    // 添加自定义筛选器配置控件
    // ...
}
```

## 📚 API文档

### 🏠 MainWindow 类

主窗口类，负责用户界面的创建和交互处理。

#### 📋 主要功能
- UI界面初始化和布局管理
- 筛选选项界面的创建和管理
- 文件类型后缀名的动态显示
- 用户交互处理和输入验证
- 线程管理和任务调度
- 进度更新显示和状态反馈

#### 🔧 核心方法

```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 文件传输相关
    void addSourceFolder();           // 添加源文件夹
    void selectTargetDirectory();     // 选择目标目录
    void startTransfer();            // 开始文件传输
    void updateProgress(int value);   // 更新进度条
    
    // 文件管理相关
    void loadDirectoryContent(const QString& path);  // 加载目录内容
    void copySelectedItems();         // 复制选中项目
    void deleteSelectedItems();       // 删除选中项目
    void renameSelectedItem();        // 重命名选中项目
    void findFiles();                // 查找文件
    void showFileProperties();        // 显示文件属性

private:
    void initUI();                   // 初始化用户界面
    void initFileTransferUI();       // 初始化文件传输界面
    void initFileManagementUI();     // 初始化文件管理界面
    void updateFileTypeExtensionsDisplay();  // 更新文件类型显示
    QStringList getFileTypeExtensions(const QString& type);  // 获取文件类型扩展名
    qint64 convertSizeToBytes(double size, const QString& unit);  // 转换大小单位
};
```

### ⚙️ FileTransferWorker 类

文件转移工作线程，负责执行实际的文件转移操作。

#### 📋 主要功能
- 文件转移逻辑实现
- 智能文件筛选处理
- 进度信息反馈
- 错误处理和报告

#### 🔧 核心方法

```cpp
class FileTransferWorker : public QObject {
    Q_OBJECT

public:
    explicit FileTransferWorker(QObject *parent = nullptr);
    
    // 传输控制
    void startTransfer(const QStringList& sourceFolders,
                      const QString& targetDirectory,
                      TransferMode mode,
                      bool overwriteFiles,
                      const FilterOptions& filterOptions);
    
    // 筛选方法
    bool passesFilter(const QString& filePath, const FilterOptions& options);
    bool matchesFileType(const QString& filePath, const QStringList& extensions);
    bool matchesFileSize(qint64 fileSize, qint64 minSize, qint64 maxSize);
    bool matchesModificationTime(const QDateTime& fileTime, 
                                const QDateTime& startTime, 
                                const QDateTime& endTime);
    bool matchesNamePattern(const QString& fileName, 
                           const QString& pattern, 
                           bool useRegex);
    bool isExcluded(const QString& fileName, const QStringList& excludeList);
    
    // 工具方法
    QString generateUniqueFileName(const QString& filePath);
    int calculateTotalFiles(const QStringList& sourceFolders, 
                           const FilterOptions& options);

signals:
    void progressUpdated(int percentage);     // 进度更新信号
    void statusUpdated(const QString& status); // 状态更新信号
    void transferCompleted(int totalFiles, int successCount); // 传输完成信号
    void errorOccurred(const QString& error); // 错误发生信号

private:
    void copyDirectoryRecursively(const QString& sourceDir, 
                                 const QString& targetDir,
                                 const FilterOptions& options);
    void copyFilesFlattened(const QString& sourceDir, 
                           const QString& targetDir,
                           const FilterOptions& options);
    int countFilesInDirectory(const QString& dirPath, 
                             const FilterOptions& options);
};
```

### 📊 数据结构

#### FilterOptions 结构体

```cpp
struct FilterOptions {
    // 筛选开关
    bool enableFileTypeFilter = false;   // 启用文件类型筛选
    bool enableNameFilter = false;       // 启用文件名筛选
    bool enableSizeFilter = false;       // 启用文件大小筛选
    bool enableTimeFilter = false;       // 启用修改时间筛选
    bool enableExcludeFilter = false;    // 启用排除列表筛选
    
    // 筛选条件
    QStringList allowedExtensions;       // 允许的文件扩展名
    QString namePattern;                 // 文件名匹配模式
    bool useRegex = false;              // 使用正则表达式
    qint64 minSize = 0;                 // 最小文件大小(字节)
    qint64 maxSize = LLONG_MAX;         // 最大文件大小(字节)
    QDateTime startTime;                // 修改时间范围开始
    QDateTime endTime;                  // 修改时间范围结束
    QStringList excludeList;            // 排除列表
};
```

#### TransferMode 枚举

```cpp
enum class TransferMode {
    KeepStructure,    // 保持原有文件夹结构
    FlattenFiles      // 扁平化转移，只提取文件
};
```

### 🔄 信号槽连接示例

```cpp
// 在 MainWindow 构造函数中
FileTransferWorker* worker = new FileTransferWorker(this);

// 连接进度更新信号
connect(worker, &FileTransferWorker::progressUpdated,
        this, &MainWindow::updateProgress);

// 连接状态更新信号
connect(worker, &FileTransferWorker::statusUpdated,
        statusLabel, &QLabel::setText);

// 连接完成信号
connect(worker, &FileTransferWorker::transferCompleted,
        this, &MainWindow::onTransferCompleted);

// 连接错误信号
connect(worker, &FileTransferWorker::errorOccurred,
        this, &MainWindow::showErrorMessage);
```

## ❓ 常见问题

### 🔧 编译问题

**Q: 编译时出现 "Qt5Core.dll not found" 错误？**

A: 这是因为系统找不到Qt运行库。解决方法：
```bash
# 方法1: 添加Qt bin目录到PATH环境变量
set PATH=C:\Qt\5.15.2\mingw81_64\bin;%PATH%

# 方法2: 使用windeployqt工具部署依赖
windeployqt.exe FileTransferTool.exe
```

**Q: 编译时出现 "mingw32-make: command not found"？**

A: MinGW编译器未正确安装或配置。请：
1. 确保已安装MinGW
2. 将MinGW的bin目录添加到PATH
3. 重启命令行工具

### 🚀 使用问题

**Q: 为什么文件传输速度很慢？**

A: 可能的原因和解决方案：
- **大量小文件**: 这是正常现象，文件系统操作开销较大
- **网络驱动器**: 网络传输本身较慢，建议使用本地磁盘
- **杀毒软件**: 实时扫描会影响速度，可临时关闭
- **磁盘碎片**: 整理磁盘碎片可提高性能

**Q: 传输过程中程序崩溃怎么办？**

A: 请检查以下几点：
1. 确保有足够的磁盘空间
2. 检查文件权限是否正确
3. 避免传输正在使用的文件
4. 更新到最新版本

**Q: 如何处理文件名包含特殊字符的情况？**

A: 程序已内置处理机制：
- 自动转换不兼容字符
- 生成唯一文件名避免冲突
- 详细错误日志记录问题文件

### 🔍 筛选问题

**Q: 正则表达式不生效？**

A: 请检查：
1. 确保启用了"使用正则表达式"选项
2. 验证正则表达式语法是否正确
3. 使用在线正则表达式测试工具验证

**Q: 文件大小筛选不准确？**

A: 注意单位换算：
- 1 KB = 1024 字节
- 1 MB = 1024 KB
- 1 GB = 1024 MB

## 📈 版本历史

### v3.2.0 (2024-01-15) - 当前版本

#### 🆕 新增功能
- ✨ 完整的文件管理功能模块
- 🗂️ 文件浏览器界面，支持多驱动器切换
- 📋 文件操作：复制、剪切、粘贴、删除、重命名
- 🔍 文件查找功能，支持通配符搜索
- ℹ️ 文件属性查看功能
- 🖱️ 右键上下文菜单
- 🎨 智能文件筛选系统重构
- 📊 文件类型后缀名动态显示

#### 🔧 改进优化
- 🚀 提升大文件传输性能
- 🛡️ 增强错误处理机制
- 🎨 改进用户界面设计
- 📝 完善代码注释和文档

#### 🐛 问题修复
- 修复文件类型匹配逻辑bug
- 解决内存泄漏问题
- 修复进度条显示不准确的问题

### v3.1.0 (2023-12-01)

#### 🆕 新增功能
- 🔍 智能文件筛选系统
- 📊 实时进度显示
- 🔄 文件冲突处理

#### 🔧 改进优化
- ⚡ 多线程性能优化
- 🎨 界面布局改进

### v3.0.0 (2023-10-15)

#### 🆕 新增功能
- 🏗️ 全新架构设计
- 📁 多源文件夹支持
- 🔄 双传输模式

### v2.x.x 系列
- v2.1.0: 添加基础筛选功能
- v2.0.0: 重构核心传输引擎

### v1.x.x 系列
- v1.2.0: 添加进度显示
- v1.1.0: 优化用户界面
- v1.0.0: 初始版本发布

## 🤝 贡献指南

### 🔄 贡献流程

1. **Fork 项目**
   ```bash
   # 在 GitHub 上点击 Fork 按钮
   git clone https://github.com/your-username/FileTransferTool.git
   ```

2. **创建功能分支**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **提交更改**
   ```bash
   git add .
   git commit -m "feat: add your feature description"
   ```

4. **推送分支**
   ```bash
   git push origin feature/your-feature-name
   ```

5. **创建 Pull Request**
   - 在 GitHub 上创建 PR
   - 详细描述更改内容
   - 等待代码审查

### 📝 提交规范

使用 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

```
type(scope): description

[optional body]

[optional footer]
```

**类型说明：**
- `feat`: 新功能
- `fix`: 问题修复
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建工具或辅助工具的变动

**示例：**
```
feat(filter): add regex support for filename filtering

Implement regular expression matching for filename patterns.
This allows users to use more complex matching rules.

Closes #123
```

### 🐛 问题报告

发现bug时，请提供以下信息：

1. **环境信息**
   - 操作系统版本
   - Qt版本
   - 编译器版本

2. **重现步骤**
   - 详细的操作步骤
   - 预期结果
   - 实际结果

3. **相关文件**
   - 错误截图
   - 日志文件
   - 示例文件（如适用）

### 💡 功能建议

提交功能建议时，请说明：

1. **使用场景**: 什么情况下需要这个功能
2. **预期行为**: 功能应该如何工作
3. **替代方案**: 是否有其他解决方案
4. **优先级**: 功能的重要程度

## 🚀 后续扩展功能

以下功能计划在未来版本中实现，以进一步提升用户体验和工具实用性：

### 🎯 v4.0.0 计划功能

#### 📝 文件内容处理
- **📄 文件预览**: 支持常见文件格式的快速预览
  - 文本文件 (.txt, .md, .log)
  - 图片文件 (.jpg, .png, .gif, .bmp)
  - PDF文档预览
- **✏️ 简单编辑**: 内置文本编辑器，支持基本编辑功能
- **🔍 文件内容搜索**: 在文件内容中搜索关键词

#### 🗜️ 压缩与解压
- **📦 文件压缩**: 支持创建ZIP、RAR、7Z格式压缩包
- **📂 解压缩**: 支持解压常见压缩格式
- **🔄 批量压缩**: 批量压缩多个文件或文件夹

#### 🔄 高级同步功能
- **☁️ 云存储同步**: 支持OneDrive、Google Drive等云服务
- **🌐 FTP/SFTP**: 远程服务器文件传输
- **📱 移动设备同步**: 与Android/iOS设备文件同步
- **⏰ 定时同步**: 设置自动同步任务

### 🎯 v4.1.0 计划功能

#### 🔒 安全功能
- **🔐 文件加密**: AES-256加密保护敏感文件
- **🔓 解密功能**: 支持加密文件的解密
- **🔑 密码管理**: 安全的密码存储和管理
- **🛡️ 文件完整性校验**: MD5/SHA256校验和验证

#### 🔍 智能分析
- **🔄 重复文件检测**: 基于内容的重复文件识别
- **📊 磁盘空间分析**: 可视化磁盘使用情况
- **📈 文件统计**: 详细的文件类型和大小统计
- **🧹 清理建议**: 智能清理建议和垃圾文件检测

### 🎯 v4.2.0 计划功能

#### 🎨 用户体验增强
- **🖱️ 拖拽支持**: 完整的拖拽操作支持
- **🎨 主题系统**: 多种界面主题选择
- **🌍 多语言支持**: 中文、英文、日文等多语言界面
- **⌨️ 快捷键自定义**: 用户自定义快捷键

#### 🔧 高级功能
- **📋 任务队列**: 文件操作任务队列管理
- **⏸️ 断点续传**: 大文件传输断点续传
- **📝 操作历史**: 详细的操作历史记录
- **🔄 撤销/重做**: 文件操作的撤销和重做功能

#### 🔌 扩展性
- **🧩 插件系统**: 支持第三方插件扩展
- **🔧 API接口**: 提供命令行和API接口
- **📊 自定义筛选器**: 用户自定义筛选规则
- **🎯 批处理脚本**: 支持批处理脚本执行

### 🎯 长期规划

#### 🌐 跨平台支持
- **🐧 Linux版本**: 完整的Linux桌面版本
- **🍎 macOS版本**: 原生macOS应用
- **📱 移动版本**: Android/iOS移动应用

#### 🤖 智能化功能
- **🧠 AI文件分类**: 基于AI的智能文件分类
- **🔮 智能建议**: 基于使用习惯的操作建议
- **📊 使用分析**: 文件使用模式分析

## 📞 联系我们

### 🐛 问题反馈
- **GitHub Issues**: [提交问题](https://github.com/your-repo/FileTransferTool/issues)
- **邮箱**: support@filetransfertool.com
- **QQ群**: 123456789

### 💬 社区交流
- **官方论坛**: [https://forum.filetransfertool.com](https://forum.filetransfertool.com)
- **微信群**: 扫描二维码加入
- **Telegram**: [@FileTransferTool](https://t.me/FileTransferTool)

### 🤝 商业合作
- **商务邮箱**: business@filetransfertool.com
- **技术支持**: tech@filetransfertool.com

## 🙏 致谢

感谢以下开源项目和贡献者：

- **Qt Framework**: 提供强大的跨平台GUI框架
- **MinGW**: 提供Windows下的GCC编译环境
- **GitHub**: 提供代码托管和协作平台
- **所有贡献者**: 感谢每一位为项目贡献代码和建议的开发者

特别感谢测试用户和反馈用户，你们的建议让这个工具变得更好！

## 📄 许可证

```
MIT License

Copyright (c) 2024 FileTransferTool Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### 📋 许可证说明

本项目采用 **MIT 许可证**，这意味着：

#### ✅ 允许的使用方式
- 🆓 **免费使用**: 个人和商业用途均可免费使用
- 🔄 **修改代码**: 可以修改源代码以满足特定需求
- 📦 **重新分发**: 可以重新分发原始或修改后的版本
- 💼 **商业使用**: 可以在商业项目中使用
- 🔒 **私有使用**: 可以在私有项目中使用而不公开源码

#### 📝 使用条件
- 📄 **保留版权声明**: 在所有副本中保留原始版权声明
- 📋 **包含许可证**: 在分发时包含完整的许可证文本
- 🚫 **免责声明**: 软件按"现状"提供，不提供任何形式的保证

#### 🚨 免责声明
- 作者不对软件的使用承担任何责任
- 用户需自行承担使用风险
- 建议在重要数据操作前进行备份

---

<div align="center">

**🌟 如果这个项目对你有帮助，请给我们一个 Star！🌟**

[![GitHub stars](https://img.shields.io/github/stars/your-repo/FileTransferTool.svg?style=social&label=Star)](https://github.com/your-repo/FileTransferTool)
[![GitHub forks](https://img.shields.io/github/forks/your-repo/FileTransferTool.svg?style=social&label=Fork)](https://github.com/your-repo/FileTransferTool/fork)
[![GitHub watchers](https://img.shields.io/github/watchers/your-repo/FileTransferTool.svg?style=social&label=Watch)](https://github.com/your-repo/FileTransferTool)

**📧 联系我们 | 🐛 报告问题 | 💡 功能建议 | 🤝 参与贡献**

</div>