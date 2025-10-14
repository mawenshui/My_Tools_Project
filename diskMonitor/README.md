# 磁盘监控器 (Disk Monitor)

![Version](https://img.shields.io/badge/version-v1.2.1-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)
![Qt](https://img.shields.io/badge/Qt-5.12.8+-green.svg)
![License](https://img.shields.io/badge/license-MIT-orange.svg)

一个基于Qt框架开发的实时磁盘使用情况监控应用程序，提供直观的可视化界面、完整的进程监控功能和强大的数据分析能力。

## 📋 项目概述

磁盘监控器是一个功能强大的系统监控工具，能够：
- 🔍 **实时监控**所有驱动器的使用状态
- 📊 **进程分析**显示每个驱动器上的活跃进程
- 📈 **数据可视化**提供丰富的图表和趋势分析
- 💾 **历史记录**完整的数据存储和导出功能
- 🎯 **智能告警**基于使用率的颜色编码系统
- 🔧 **系统集成**完整的系统托盘功能

## ✨ 主要功能

### 🖥️ 实时磁盘监控
- 显示所有可用驱动器的使用情况
- 智能颜色编码（绿色→黄色→橙色→红色）
- 可配置的刷新间隔（1-30秒，默认10秒）
- 响应式UI布局，支持窗口大小调整

### 🔍 进程监控与分析
- **多线程架构**：独立线程监控，不阻塞UI
- **详细信息**：PID、内存使用、磁盘读写统计、CPU使用率
- **智能匹配**：自动匹配进程与对应驱动器
- **系统进程识别**：自动标记关键系统进程
- **交互功能**：双击打开进程位置、右键菜单操作
- **完善排序**：支持所有列的数值/文本排序

### 📊 数据可视化
- **实时图表**：当前磁盘使用率动态显示
- **历史趋势**：长期使用情况变化分析
- **多驱动器对比**：同时比较多个驱动器
- **数据传输图表**：磁盘读写活动可视化
- **进程活动图表**：进程磁盘使用情况展示
- **图表导出**：支持保存为图片格式

### 💾 历史数据管理
- **SQLite数据库**：可靠的历史数据存储
- **数据导出**：支持CSV和Excel格式
- **灵活查询**：按时间范围和驱动器筛选
- **自动清理**：可配置的数据保留策略
- **专用查看器**：直观的历史数据查看界面

### 🔧 系统集成
- **系统托盘**：完整的托盘功能，支持后台运行
- **托盘设置**：最小化到托盘、关闭到托盘等配置
- **消息通知**：重要事件的托盘通知
- **快速操作**：托盘右键菜单提供便捷访问

## 🚀 快速开始

### 系统要求
- **操作系统**：Windows 10/11
- **运行时**：Qt 5.12.8 或更高版本
- **内存**：至少 50MB 可用内存
- **磁盘空间**：约 20MB

### 安装方式

#### 方式一：直接运行（推荐）
1. 下载最新版本的 `diskMonitor.exe`
2. 双击运行即可，无需安装
3. 首次运行会自动创建配置文件和数据库

#### 方式二：从源码编译
```bash
# 克隆项目
git clone [项目地址]
cd diskMonitor

# 使用qmake构建
qmake diskMonitor.pro
mingw32-make

# 运行程序
.\bin\diskMonitor.exe
```

### 开发环境配置
- **IDE**：Qt Creator（推荐）
- **编译器**：MinGW 7.3.0 64位
- **构建系统**：qmake
- **Qt模块**：core, gui, widgets, sql, charts

## 📖 使用指南

### 基本操作
1. **启动程序**：双击 `diskMonitor.exe` 启动
2. **选择磁盘**：点击任意磁盘查看详细信息
3. **查看进程**：在右侧进程列表中查看活跃进程
4. **排序数据**：点击列标题进行升序/降序排序
5. **查看历史**：菜单栏 → 历史数据 → 查看历史记录
6. **图表分析**：点击图表按钮查看可视化数据

### 高级功能
- **导出数据**：历史数据对话框 → 导出 → 选择格式
- **自定义设置**：菜单栏 → 设置 → 调整刷新间隔等参数
- **系统托盘**：最小化程序到系统托盘后台运行
- **图表配置**：图表查看器中自定义时间范围和样式

## 🏗️ 技术架构

### 核心组件
- **MainWindow**：主窗口类，UI管理和用户交互
- **ProcessMonitor**：进程监控器，系统进程信息收集
- **HistoryManager**：历史数据管理器，数据库操作
- **ChartManager**：图表管理器，数据可视化
- **HistoryDialog**：历史数据查看对话框
- **ChartDialog**：图表查看器对话框

### 技术特性
- **多线程设计**：主线程负责UI，监控线程负责数据收集
- **Windows API集成**：深度集成系统API获取进程信息
- **Qt Charts集成**：专业级图表可视化功能
- **SQLite数据库**：轻量级数据持久化解决方案
- **事件驱动架构**：基于Qt信号槽的组件通信

### 数据结构
```cpp
// 驱动器信息结构
struct DriveInfo {
    QString driveLetter;              // 驱动器盘符
    QString displayName;              // 显示名称
    qint64 totalSpace;               // 总空间
    qint64 usedSpace;                // 已用空间
    double usagePercentage;          // 使用百分比
    QList<ProcessInfo> topProcesses; // 相关进程列表
    // ... 更多字段
};

// 进程信息结构
struct ProcessInfo {
    QString processName;        // 进程名称
    quint32 processId;         // 进程ID
    qint64 workingSetSize;     // 内存使用
    qint64 diskReadBytes;      // 磁盘读取
    qint64 diskWriteBytes;     // 磁盘写入
    double cpuUsage;           // CPU使用率
    // ... 更多字段
};
```

## 📊 性能指标

- **响应时间**：界面更新延迟 < 100ms
- **内存使用**：运行时内存占用 < 50MB
- **CPU使用**：空闲时CPU占用 < 1%
- **监控能力**：支持同时监控100+进程
- **数据处理**：实时更新，支持1秒刷新间隔

## 🗂️ 项目结构

```
diskMonitor/
├── 📁 src/                    # 源代码目录
│   ├── main.cpp              # 程序入口
│   ├── mainwindow.cpp/.h     # 主窗口
│   ├── processmonitor.cpp/.h # 进程监控
│   ├── historymanager.cpp/.h # 历史数据管理
│   ├── chartmanager.cpp/.h   # 图表管理
│   └── *.ui                  # UI设计文件
├── 📁 bin/                    # 可执行文件
│   ├── diskMonitor.exe       # 主程序
│   ├── 📁 config/            # 配置文件
│   ├── 📁 database/          # 数据库文件
│   └── 📁 log/               # 运行日志
├── 📁 build/                  # 构建目录
├── diskMonitor.pro           # 项目配置
├── README.md                 # 本文档
└── 项目规范文档.md           # 详细技术文档
```

## 🔄 版本历史

### v1.2.1 (当前版本) - 2024年6月
- ✅ **排序功能修复**：完善进程列表排序算法
- ✅ **数据存储优化**：改进数值数据存储机制
- ✅ **用户体验提升**：所有列支持正确排序

### v1.2 - 图表可视化功能
- ✅ **图表系统**：完整的图表管理和显示框架
- ✅ **多种图表类型**：实时、历史、对比、传输、活动图表
- ✅ **图表功能增强**：导出、实时更新、样式自定义

### v1.1 - 历史数据记录 + 系统托盘功能
- ✅ **历史数据管理**：SQLite数据库集成
- ✅ **数据导出功能**：CSV和Excel格式支持
- ✅ **系统托盘集成**：完整的后台运行支持

### v1.0 - 基础功能
- ✅ **磁盘监控**：实时显示驱动器使用情况
- ✅ **进程监控**：多线程进程信息收集
- ✅ **UI交互**：磁盘选择视觉反馈

## 🤝 贡献指南

### 开发环境设置
1. 安装 Qt Creator 和 Qt 5.12.8+
2. 配置 MinGW 7.3.0 64位编译器
3. 克隆项目并用 Qt Creator 打开 `diskMonitor.pro`
4. 构建并运行项目

### 代码规范
- **命名**：类名PascalCase，函数名camelCase
- **缩进**：4个空格，不使用Tab
- **注释**：中文注释，重要函数需详细说明
- **提交**：清晰的commit信息，遵循约定式提交

### 提交流程
1. Fork 项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

## 📝 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 📞 联系方式

- **项目主页**：[GitHub Repository]
- **问题反馈**：[GitHub Issues]
- **技术文档**：查看 `项目规范文档.md`

## 🙏 致谢

感谢以下开源项目和技术：
- [Qt Framework](https://www.qt.io/) - 跨平台应用程序框架
- [Qt Charts](https://doc.qt.io/qt-5/qtcharts-index.html) - 图表可视化组件
- [SQLite](https://www.sqlite.org/) - 轻量级数据库引擎
- Windows API - 系统信息获取接口

---

**磁盘监控器** - 让磁盘管理变得简单高效 🚀