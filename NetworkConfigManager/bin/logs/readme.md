# Logger日志记录类详细使用说明

## 1. 类概述

`Logger` 类是一个线程安全的日志记录系统，专为 Qt 应用程序设计。它提供了以下功能：

- 支持不同日志等级（DEBUG、INFO、WARNING、ERROR、CRITICAL）
- 日志按日期自动分割文件
- 线程安全操作
- 简单易用的接口
- 自动创建日志目录

## 2. 快速开始

### 2.1 基本使用步骤

1. 包含头文件
2. 初始化日志系统
3. 记录日志
4. （可选）关闭日志系统

```
#include "Logger.h"

int main(int argc, char *argv[])
{
    // 初始化日志系统
    Logger::instance()->init(); // 使用默认设置
    
    // 记录日志
    Logger::debug("Application started");
    Logger::info("Loading configuration...");
    Logger::warning("Configuration file not found, using defaults");
    
    // ... 应用程序逻辑 ...
    
    Logger::info("Application shutting down");
    
    // 关闭日志系统（可选）
    Logger::instance()->shutdown();
    
    return 0;
}
```

## 3. 详细使用方法

### 3.1 初始化日志系统

在使用日志功能前，必须先初始化日志系统：

```
// 使用默认设置（当前目录下的logs文件夹，日志文件前缀为"app"）
Logger::instance()->init();

// 自定义日志目录和文件前缀
Logger::instance()->init("/var/log/myapp", "myapp");
```

**参数说明：**

- `logDir`：日志文件存储目录，如果为空则使用默认目录（当前目录下的logs文件夹）
- `logFilePrefix`：日志文件前缀，默认为"app"

**返回值：**

- `true`：初始化成功
- `false`：初始化失败（通常是目录创建或文件打开失败）

### 3.2 记录日志

#### 3.2.1 使用便捷方法

```
// 记录DEBUG级别日志
Logger::debug("This is a debug message");

// 记录INFO级别日志
Logger::info("This is an info message");

// 记录WARNING级别日志
Logger::warning("This is a warning message");

// 记录ERROR级别日志
Logger::error("This is an error message");

// 记录CRITICAL级别日志
Logger::critical("This is a critical message");
```

#### 3.2.2 使用通用方法

```
Logger::instance()->log(Logger::DEBUG, "Debug message");
Logger::instance()->log(Logger::INFO, "Info message");
Logger::instance()->log(Logger::WARNING, "Warning message");
Logger::instance()->log(Logger::ERROR, "Error message");
Logger::instance()->log(Logger::CRITICAL, "Critical message");
```

### 3.3 关闭日志系统

在程序退出前，可以显式关闭日志系统以确保所有日志都写入文件：

```
Logger::instance()->shutdown();
```

**注意：** 这不是必须的，因为析构函数会自动调用此方法，但显式调用可以确保日志及时写入。

## 4. 日志文件格式

日志文件命名格式：

```
 [prefix]_yyyyMMdd.log
```

例如：

```
 app_20230415.log
myapp_20230415.log
```

日志条目格式：

```
 [yyyy-MM-dd hh:mm:ss.zzz] [LEVEL] message
```

示例：

```
 [2023-04-15 14:30:45.123] [INFO] Application started
[2023-04-15 14:30:45.456] [WARNING] Configuration file not found
```

## 5. 高级用法

### 5.1 检查日志系统是否初始化

```
if (!Logger::instance()->isInitialized()) {
    // 处理日志系统未初始化的情况
}
```

（注意：需要在Logger类中添加isInitialized()方法）

### 5.2 自定义日志格式

可以通过继承Logger类并重写相关方法来实现自定义日志格式。

### 5.3 多线程使用示例

```
#include <QThread>

class WorkerThread : public QThread
{
    void run() override {
        Logger::info(QString("Thread %1 started").arg(currentThreadId()));
        
        // ... 工作代码 ...
        
        Logger::info(QString("Thread %1 finished").arg(currentThreadId()));
    }
};

int main(int argc, char *argv[])
{
    Logger::instance()->init();
    
    WorkerThread threads[5];
    for (int i = 0; i < 5; ++i) {
        threads[i].start();
    }
    
    for (int i = 0; i < 5; ++i) {
        threads[i].wait();
    }
    
    return 0;
}
```

## 6. 最佳实践

1. **尽早初始化**：在应用程序启动时立即初始化日志系统
2. 合理使用日志等级：
   - DEBUG：开发调试信息
   - INFO：常规运行信息
   - WARNING：预期外但不影响程序运行的情况
   - ERROR：严重问题，但程序仍能运行
   - CRITICAL：致命错误，程序可能无法继续运行
3. **避免过度记录**：日志过多会影响性能并增加查找有用信息的难度
4. **敏感信息**：不要在日志中记录密码等敏感信息
5. **日志轮转**：对于长期运行的应用程序，考虑实现日志轮转机制

## 7. 常见问题解答

**Q: 日志文件会变得很大吗？**

A: 日志文件按天分割，每天会创建新的日志文件。如果需要限制单个文件大小，可以扩展该类实现日志轮转功能。

**Q: 在多线程环境中使用安全吗？**

A: 是的，该类使用QMutex确保线程安全，可以安全地在多线程环境中使用。

**Q: 如何更改日志的时间格式？**

A: 可以修改log()方法中的时间格式字符串，例如改为"hh:mm:ss"只显示时间。

**Q: 日志写入失败会有什么表现？**

A: 如果日志文件无法打开或写入，日志记录操作会静默失败，不会影响程序运行。

**Q: 如何实现日志同时输出到控制台和文件？**

A: 可以扩展该类，在log()方法中添加输出到控制台的代码，例如使用qDebug()。