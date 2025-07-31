# CodeVisualization 项目规范文档

## 文档信息

- **项目名称**: CodeVisualization (代码可视化统计工具)
- **文档版本**: v1.0
- **创建日期**: 2024年12月
- **最后更新**: 2024年12月
- **维护团队**: CodeVisualization Team

## 1. 项目概述

### 1.1 项目简介

CodeVisualization 是一个基于 Qt 框架开发的桌面应用程序，用于分析和统计代码项目的各种指标。该工具提供直观的图形界面，支持多种编程语言的代码分析，并能生成详细的统计报告和可视化图表。

### 1.2 项目目标

- 提供准确、高效的代码统计分析功能
- 支持多种主流编程语言
- 提供现代化、用户友好的图形界面
- 生成可视化的统计报告
- 保持良好的性能和稳定性

### 1.3 核心功能

- **代码分析**: 统计代码行数、注释行数、空白行数
- **语言识别**: 自动识别多种编程语言
- **文件统计**: 分析文件数量、大小等信息
- **可视化展示**: 图表展示统计结果
- **报告导出**: 支持 HTML 格式报告导出
- **主题管理**: 支持多种界面主题

## 2. 技术架构

### 2.1 技术栈

- **开发语言**: C++11
- **GUI框架**: Qt 5.12.8+
- **编译器**: MinGW 7.3.0+
- **构建系统**: qmake
- **IDE**: Qt Creator
- **操作系统**: Windows (主要支持)

### 2.2 Qt 模块依赖

```pro
QT += core widgets charts printsupport concurrent network svg
```

- **core**: Qt 核心功能
- **widgets**: GUI 组件
- **charts**: 图表组件
- **printsupport**: 打印支持
- **concurrent**: 多线程支持
- **network**: 网络功能
- **svg**: SVG 图形支持

### 2.3 项目结构

```
CodeVisualization/
├── main.cpp                    # 程序入口
├── mainwindow.h/cpp/ui         # 主窗口
├── CodeVisualization.pro       # 项目配置文件
├── bin/                        # 可执行文件目录
│   ├── CodeVisualization.exe
│   └── config/
│       └── app_config.json
├── build/                      # 构建输出目录
├── doc/                        # 文档目录
│   ├── Qt界面设计方案.md
│   ├── 项目开发规范文档.md
│   ├── 项目需求文档_UI优化版.md
│   └── CodeVisualization项目规范文档.md
└── src/                        # 源代码目录
    ├── core/                   # 核心功能模块
    │   ├── analyzer/           # 代码分析引擎
    │   │   ├── code_analyzer.h
    │   │   └── code_analyzer.cpp
    │   ├── config/             # 配置管理
    │   │   ├── config_manager.h
    │   │   └── config_manager.cpp
    │   ├── export/             # 导出功能
    │   └── models/             # 数据模型
    │       ├── analysis_result.h/cpp
    │       └── file_statistics.h/cpp
    └── ui/                     # 用户界面
        ├── dialogs/            # 对话框
        ├── theme/              # 主题管理
        │   ├── theme_manager.h
        │   └── theme_manager.cpp
        └── widgets/            # 自定义控件
            ├── project_selection_widget.h/cpp
            ├── statistics_widget.h/cpp
            ├── chart_widget.h/cpp
            ├── advanced_config_dialog.h/cpp
            ├── html_preview_dialog.h/cpp
            └── chart_selection_dialog.h/cpp
```

## 3. 编码规范

### 3.1 命名规范

#### 3.1.1 文件命名
- 头文件: 小写字母 + 下划线，如 `code_analyzer.h`
- 源文件: 小写字母 + 下划线，如 `code_analyzer.cpp`
- UI文件: 小写字母 + 下划线，如 `mainwindow.ui`

#### 3.1.2 类命名
- 使用帕斯卡命名法 (PascalCase)
- 示例: `CodeAnalyzer`, `MainWindow`, `ThemeManager`

#### 3.1.3 函数命名
- 使用驼峰命名法 (camelCase)
- 示例: `analyzeProject()`, `getRecentProjects()`, `setTheme()`

#### 3.1.4 变量命名
- 局部变量: 驼峰命名法，如 `projectPath`, `fileCount`
- 成员变量: `m_` 前缀 + 驼峰命名法，如 `m_analyzer`, `m_configManager`
- 静态变量: `s_` 前缀 + 驼峰命名法，如 `s_instance`
- 常量: 全大写 + 下划线，如 `MAX_FILE_SIZE`, `DEFAULT_TIMEOUT`

#### 3.1.5 枚举命名
- 枚举类型: 帕斯卡命名法，如 `AnalysisStatus`
- 枚举值: 帕斯卡命名法，如 `Idle`, `Scanning`, `Completed`

### 3.2 代码格式

#### 3.2.1 缩进和空格
- 使用 4 个空格进行缩进，不使用 Tab
- 操作符前后添加空格
- 逗号后添加空格
- 函数参数列表中的逗号后添加空格

#### 3.2.2 大括号风格
```cpp
// 推荐的大括号风格
if (condition) {
    // 代码块
}

class MyClass
{
public:
    void myFunction();
};
```

#### 3.2.3 行长度
- 每行代码不超过 120 个字符
- 超长行应适当换行，保持可读性

### 3.3 注释规范

#### 3.3.1 文件头注释
```cpp
/**
 * @file code_analyzer.h
 * @brief 代码分析器头文件
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.0.0
 */
```

#### 3.3.2 类注释
```cpp
/**
 * @brief 代码分析器类
 * 
 * 负责分析项目中的代码文件，统计各种指标
 * 支持多种编程语言的识别和分析
 * 使用多线程进行文件扫描和分析
 * 
 * @author CodeVisualization Team
 * @date 2024-01-01
 * @version 1.0.0
 */
class CodeAnalyzer : public QObject
{
    // 类实现
};
```

#### 3.3.3 函数注释
```cpp
/**
 * @brief 开始分析项目
 * @param projectPath 项目路径
 * @param options 分析选项
 * @return 是否成功启动分析
 */
bool startAnalysis(const QString &projectPath, const AnalysisOptions &options);
```

#### 3.3.4 成员变量注释
```cpp
private:
    CodeAnalyzer *m_analyzer;           ///< 代码分析器实例
    QProgressBar *m_progressBar;        ///< 进度条
    AnalysisResult m_lastResult;        ///< 最后一次分析结果
```

### 3.4 Qt 特定规范

#### 3.4.1 信号和槽
- 信号名使用动词过去式，如 `analysisCompleted`, `fileSelected`
- 槽函数使用 `on` 前缀，如 `onAnalysisCompleted`, `onFileSelected`

#### 3.4.2 Q_OBJECT 宏
- 所有继承自 QObject 的类都必须包含 Q_OBJECT 宏
- Q_OBJECT 宏应放在类声明的开始位置

#### 3.4.3 内存管理
- 优先使用 Qt 的父子对象机制进行内存管理
- 使用智能指针 (QScopedPointer, QSharedPointer) 管理资源
- 避免手动 delete，除非必要

## 4. 架构设计

### 4.1 分层架构

```
┌─────────────────────────────────────┐
│            UI Layer                 │
│  (MainWindow, Widgets, Dialogs)     │
├─────────────────────────────────────┤
│          Business Layer             │
│    (Managers, Controllers)          │
├─────────────────────────────────────┤
│           Core Layer                │
│   (Analyzer, Models, Utils)         │
├─────────────────────────────────────┤
│          Data Layer                 │
│    (Config, Export, Storage)        │
└─────────────────────────────────────┘
```

### 4.2 核心组件

#### 4.2.1 CodeAnalyzer (代码分析器)
- **职责**: 扫描和分析代码文件
- **特性**: 多线程处理、进度报告、错误处理
- **接口**: 异步分析接口，信号槽通信

#### 4.2.2 ConfigManager (配置管理器)
- **职责**: 应用配置的读取和保存
- **特性**: 单例模式、JSON 格式配置
- **配置项**: 最近项目、界面设置、分析选项

#### 4.2.3 ThemeManager (主题管理器)
- **职责**: 界面主题的管理和切换
- **特性**: 动态主题切换、样式表管理
- **支持**: 明亮主题、暗黑主题

### 4.3 数据模型

#### 4.3.1 AnalysisResult
- 存储分析结果的完整数据
- 包含文件统计、语言统计、时间信息

#### 4.3.2 FileStatistics
- 单个文件的统计信息
- 包含行数、大小、语言类型等

## 5. 开发流程

### 5.1 版本控制

#### 5.1.1 分支策略
- **main**: 主分支，稳定版本
- **develop**: 开发分支，集成新功能
- **feature/***: 功能分支，开发新功能
- **hotfix/***: 热修复分支，紧急修复

#### 5.1.2 提交规范
```
<type>(<scope>): <subject>

<body>

<footer>
```

类型说明:
- **feat**: 新功能
- **fix**: 修复 bug
- **docs**: 文档更新
- **style**: 代码格式调整
- **refactor**: 代码重构
- **test**: 测试相关
- **chore**: 构建过程或辅助工具的变动

### 5.2 构建和部署

#### 5.2.1 构建配置
```pro
# 版本信息
VERSION = 1.1.3
TARGET = CodeVisualization
DESTDIR = $$PWD/bin

# 输出目录
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
RCC_DIR = build/rcc
UI_DIR = build/ui
```

#### 5.2.2 部署清单
- 可执行文件: `CodeVisualization.exe`
- 配置文件: `config/app_config.json`
- Qt 运行时库
- 必要的 DLL 文件

### 5.3 测试策略

#### 5.3.1 单元测试
- 使用 Qt Test 框架
- 测试核心算法和数据模型
- 覆盖率目标: 80%+

#### 5.3.2 集成测试
- 测试组件间的交互
- 验证完整的工作流程

#### 5.3.3 UI 测试
- 手动测试用户界面
- 验证用户体验和交互逻辑

## 6. 性能要求

### 6.1 响应时间
- 界面响应时间 < 100ms
- 小型项目分析时间 < 5s
- 大型项目分析时间 < 30s

### 6.2 资源占用
- 内存占用 < 200MB (正常使用)
- CPU 占用 < 50% (分析期间)
- 磁盘空间 < 50MB (安装包)

### 6.3 并发处理
- 支持多线程文件扫描
- 避免 UI 线程阻塞
- 提供取消操作功能

## 7. 安全要求

### 7.1 输入验证
- 验证文件路径的有效性
- 检查文件权限和访问性
- 防止路径遍历攻击

### 7.2 错误处理
- 优雅处理文件访问错误
- 提供详细的错误信息
- 记录异常情况到日志

### 7.3 数据保护
- 不存储敏感的源代码内容
- 配置文件使用安全的存储位置
- 支持配置文件的备份和恢复

## 8. 维护和扩展

### 8.1 代码维护
- 定期进行代码审查
- 及时更新依赖库版本
- 保持文档的同步更新

### 8.2 功能扩展
- 模块化设计，便于添加新功能
- 插件架构支持第三方扩展
- 保持向后兼容性

### 8.3 国际化支持
- 使用 Qt 的国际化机制
- 支持多语言界面
- 提供翻译文件管理

## 9. 文档要求

### 9.1 技术文档
- API 文档 (Doxygen 生成)
- 架构设计文档
- 部署和安装指南

### 9.2 用户文档
- 用户使用手册
- 功能介绍和教程
- 常见问题解答

### 9.3 开发文档
- 项目规范文档 (本文档)
- 开发环境搭建指南
- 贡献者指南

## 10. 质量保证

### 10.1 代码质量
- 遵循编码规范
- 进行代码审查
- 使用静态分析工具

### 10.2 测试质量
- 完善的测试用例
- 自动化测试流程
- 性能测试和压力测试

### 10.3 发布质量
- 版本发布检查清单
- 用户反馈收集机制
- 问题跟踪和修复流程

---

**文档维护**: 本文档应随项目发展持续更新，确保与实际实现保持一致。

**联系方式**: 如有疑问或建议，请联系 CodeVisualization 开发团队。