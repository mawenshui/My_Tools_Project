# FaultAlarmWidget 类规范文档

## 概述

`FaultAlarmWidget` 是一个基于 Qt 的故障告警数据帧处理工具类，继承自 `QWidget`。该类实现了故障告警数据帧的创建、编辑、发送和管理功能，支持 UDP 网络传输，并提供了完整的用户界面。

该工具特别支持符合国家标准的唯一标识码格式，采用 Data Matrix(DM) 码制，ECC200 类型，C40 编码方案，24位固定长度代码结构。唯一标识码由组织机构代码、生产日期、序列码和校验码组成，确保设备标识的唯一性和可追溯性。

## 类信息

- **类名**: `FaultAlarmWidget`
- **继承**: `QWidget`
- **头文件**: `faultAlarmWidget.h`
- **实现文件**: `faultAlarmWidget.cpp`
- **命名空间**: 无

## 功能特性

### 核心功能
1. **数据帧构建**: 按照故障告警协议格式构建完整的数据帧
2. **网络发送**: 支持 UDP 单帧发送和连续发送
3. **数据配置**: 提供图形化界面配置各种数据参数
4. **格式支持**: 支持十进制和十六进制数值显示格式
5. **预设管理**: 支持配置的保存和加载
6. **实时日志**: 提供详细的操作日志和状态反馈

### 界面特性
- 分割式布局，左侧配置区域，右侧日志显示
- 深色主题风格，与项目整体风格一致
- 响应式设计，支持窗口大小调整
- 丰富的输入验证和用户提示

## 数据结构

### FaultAlarmData 结构体

```cpp
struct FaultAlarmData {
    // 帧头信息
    quint8 control = 0x01;              // 控制字节 (0或1)
    quint16 dataLength = 0;             // 数据域长度
    quint16 topicNumber = 0;            // 主题号
    quint8 sourceDevice = 0;            // 源设备号
    quint8 destDevice = 0;              // 目的设备号
    quint32 timestamp = 0;              // 数据产生时刻
    
    // 数据域信息
    quint8 alarmIdentifier = 0x01;      // 告警标识符 (0x01:故障码, 0x02:预警码)
    quint16 faultCode = 0;              // 故障码/预警码
    quint8 isolationFlag = 0;           // 是否需要隔离标志位
    quint8 faultLevel = 0;              // 故障等级
    float warningValue = 0.0f;          // 数据预警数值
    float warningThreshold = 0.0f;      // 数据预警判定阈值
    QString uniqueId = "";              // 唯一标识码 (24字节)
};
```

## 公共接口

### 构造函数和析构函数

```cpp
explicit FaultAlarmWidget(QWidget *parent = nullptr);
~FaultAlarmWidget();
```

- **构造函数**: 初始化界面、网络组件和配置
- **析构函数**: 保存配置，清理资源

## 槽函数接口

### 发送配置相关

| 函数名 | 功能描述 |
|--------|----------|
| `onSendSingleFrame()` | 发送单帧数据 |
| `onStartContinuousSend()` | 开始连续发送 |
| `onStopContinuousSend()` | 停止连续发送 |
| `onClearLog()` | 清空日志显示 |

### 数据配置相关

| 函数名 | 功能描述 |
|--------|----------|
| `onAlarmTypeChanged()` | 告警类型改变处理 |
| `onGenerateUniqueId()` | 生成唯一标识码 |
| `onLoadPreset()` | 加载预设配置 |
| `onSavePreset()` | 保存预设配置 |
| `onResetData()` | 重置数据为默认值 |
| `onFormatChanged()` | 数值格式改变处理 |

### 分段式唯一标识符相关

| 函数名 | 功能描述 |
|--------|----------|
| `onUniqueIdSegmentChanged()` | 分段输入字段变化处理 |
| `onCalculateCheckCode()` | 计算校验码 |
| `onUniqueIdResultChanged()` | 组合结果字段变化处理 |

### 定时器相关

| 函数名 | 功能描述 |
|--------|----------|
| `onSendTimer()` | 定时发送处理 |

## 私有方法

### 初始化函数

| 函数名 | 功能描述 |
|--------|----------|
| `initUI()` | 初始化用户界面 |
| `initConnections()` | 初始化信号槽连接 |
| `loadStyleSheet()` | 加载样式表 |

### UI创建函数

| 函数名 | 返回类型 | 功能描述 |
|--------|----------|----------|
| `createDataConfigWidget()` | `QWidget*` | 创建数据配置部件 |
| `createSendConfigWidget()` | `QWidget*` | 创建发送配置部件 |
| `createLogWidget()` | `QWidget*` | 创建日志显示部件 |

### 数据处理函数

| 函数名 | 返回类型 | 功能描述 |
|--------|----------|----------|
| `buildDataFrame()` | `QByteArray` | 构建完整数据帧 |
| `calculateChecksum(const QByteArray &data)` | `quint16` | 计算校验和 |
| `updateDataLength()` | `void` | 更新数据长度 |
| `updateTimestamp()` | `void` | 更新时间戳 |
| `generateUniqueId()` | `QString` | 生成唯一标识码 |

### UI更新函数

| 函数名 | 功能描述 |
|--------|----------|
| `updateUI()` | 更新界面显示 |
| `updateSendButtonStates()` | 更新发送按钮状态 |
| `appendLog(const QString &level, const QString &message)` | 添加日志条目 |

### 配置管理函数

| 函数名 | 返回类型 | 功能描述 |
|--------|----------|----------|
| `saveConfiguration()` | `void` | 保存配置到文件 |
| `loadConfiguration()` | `void` | 从文件加载配置 |
| `dataToJson()` | `QJsonObject` | 数据转换为JSON |
| `jsonToData(const QJsonObject &json)` | `void` | JSON转换为数据 |

## 成员变量

### 网络相关
- `QUdpSocket *m_udpSocket`: UDP套接字
- `QTimer *m_sendTimer`: 发送定时器

### 数据相关
- `FaultAlarmData m_currentData`: 当前数据
- `bool m_isContinuousSending`: 连续发送状态

### UI组件
- `QWidget *m_dataConfigWidget`: 数据配置部分
- `QWidget *m_sendConfigWidget`: 发送配置部分
- `QWidget *m_logWidget`: 日志显示部分

### 数据配置控件
- `QComboBox *m_controlCombo`: 控制位选择
- `QComboBox *m_formatCombo`: 数值格式选择
- `QComboBox *m_alarmTypeCombo`: 告警类型选择
- `QSpinBox *m_faultCodeSpin`: 故障码输入
- `QComboBox *m_isolationCombo`: 隔离标志选择
- `QComboBox *m_faultLevelCombo`: 故障等级选择
- `QDoubleSpinBox *m_warningValueSpin`: 预警数值输入
- `QDoubleSpinBox *m_warningThresholdSpin`: 预警阈值输入
- `QSpinBox *m_topicNumberSpin`: 主题号输入
- `QSpinBox *m_sourceDeviceSpin`: 源设备号输入
- `QSpinBox *m_destDeviceSpin`: 目的设备号输入

### 分段式唯一标识符控件
- `QLineEdit *m_orgCodeEdit`: 组织机构代码输入 (4字符)
- `QLineEdit *m_productDateEdit`: 生产日期输入 (8字符，YYYYMMDD格式)
- `QLineEdit *m_serialCodeEdit`: 序列号输入 (10字符)
- `QLineEdit *m_checkCodeEdit`: 校验码输入 (2字符，只读)
- `QLineEdit *m_uniqueIdResultEdit`: 组合结果输入 (24字符完整标识符)

### 发送配置控件
- `QLineEdit *m_targetIpEdit`: 目标IP输入
- `QSpinBox *m_targetPortSpin`: 目标端口输入
- `QSpinBox *m_intervalSpin`: 发送间隔输入
- `QPushButton *m_sendSingleBtn`: 发送单帧按钮
- `QPushButton *m_startContinuousBtn`: 开始连续发送按钮
- `QPushButton *m_stopContinuousBtn`: 停止连续发送按钮

### 日志显示控件
- `QTextEdit *m_logTextEdit`: 日志显示区域
- `QPushButton *m_clearLogBtn`: 清空日志按钮

## 协议实现

### 数据帧格式

故障告警数据帧采用以下格式：

```
帧头(11字节) + 数据域(37字节) + 帧尾(2字节校验和)
```

#### 帧头结构
1. 控制字节 (1字节): 用户可选择0或1
2. 数据域长度 (2字节): 固定为37，小端序
3. 主题号 (2字节): 小端序
4. 源设备号 (1字节)
5. 目的设备号 (1字节)
6. 时间戳 (4字节): 小端序

#### 数据域结构
1. 告警标识符 (1字节): 0x01=故障码, 0x02=预警码
2. 故障码/预警码 (2字节): 小端序
3. 隔离标志 (1字节)
4. 故障等级 (1字节)
5. 预警数值 (4字节): IEEE 754浮点数
6. 预警阈值 (4字节): IEEE 754浮点数
7. 唯一标识码 (24字节): 采用Data Matrix(DM)码制，ECC200类型，C40编码方案的分段式组合标识符，结构如下：
   - 组织机构代码 (9字符): 统一社会信用代码的第九位至倒数第二位
   - 生产日期 (8字符): YYYYMMDD格式，标识设备生产日期
   - 序列码 (6字符): 含固定码S，标识设备所属分系统、设备及生产日期当天的设备序号
   - 校验码 (1字符): 前23位十进制累加和对10取余的校验结果
   
   **编码规则**:
   - 字符集: 阿拉伯数字0~9、大写字母A~Z（不含O、I）
   - 校验算法: 数字按十进制计算，字母按ASCII码值（十进制）计算
   - 唯一性要求: 各编码单位需确保编码的唯一性，不重复

#### 帧尾结构
- 校验和 (2字节): 对帧头和数据域的累加校验，小端序

## 使用方法

### 基本使用

```cpp
// 创建实例
FaultAlarmWidget *widget = new FaultAlarmWidget(parent);

// 显示窗口
widget->show();
```

### 编程接口使用

```cpp
// 发送单帧数据
widget->onSendSingleFrame();

// 开始连续发送
widget->onStartContinuousSend();

// 停止连续发送
widget->onStopContinuousSend();

// 重置数据
widget->onResetData();
```

## 配置文件

### 配置保存
- 配置自动保存到应用程序目录下的配置文件
- 支持手动保存预设配置到指定位置
- 配置格式为JSON

### 配置内容
- 网络配置 (IP地址、端口)
- 数据参数配置
- 发送间隔设置
- 界面状态

## 日志系统

### 日志级别
- **INFO**: 一般信息
- **SUCCESS**: 成功操作
- **WARNING**: 警告信息
- **ERROR**: 错误信息

### 日志功能
- 实时显示操作状态
- 彩色分级显示
- 自动滚动到最新日志
- 支持清空操作

## 依赖项

### Qt模块
- QtCore
- QtWidgets
- QtNetwork

### 项目依赖
- `alldefine.h`: 项目通用定义

## 分段式唯一标识符功能

### 功能概述
分段式唯一标识符功能将原来的24字符单一输入字段拆分为多个有意义的分段，提供更好的用户体验和数据管理能力。

### 分段结构
1. **组织机构代码** (9字符)
   - 统一社会信用代码的第九位至倒数第二位
   - 字符集: 阿拉伯数字0~9、大写字母A~Z（不含O、I）
   - 示例: "123456789", "ABCDEFGHI"

2. **生产日期** (8字符)
   - 格式: YYYYMMDD
   - 标识设备生产日期：年、月、日
   - 提供"今天"按钮快速填入当前日期
   - 示例: "20210501"

3. **序列码** (6字符)
   - 含固定码S，标识设备所属分系统、设备及生产日期当天的设备序号
   - 字符集: 阿拉伯数字0~9、大写字母A~Z（不含O、I）
   - 提供"随机"按钮生成随机序列码
   - 示例: "SG0110", "SA0001"

4. **校验码** (1字符)
   - 自动计算的校验码，只读字段
   - 前23位十进制累加和对10取余的校验结果
   - 数字按十进制计算，字母按ASCII码值（十进制）计算
   - 示例: 对于"63377234220220913SG01102"，校验码为"2"

### 功能特性

#### 自动计算
- 当任何分段字段发生变化时，自动重新计算校验码
- 校验算法: 前23位十进制累加和对10取余
- 数字按十进制值计算，字母按ASCII码值（十进制）计算
- 实时更新24字符的组合结果字段
- 确保数据的一致性和完整性

#### 双向编辑
- **分段输入**: 可以分别编辑各个分段字段
- **整体输入**: 可以直接编辑24字符的完整标识符
- **自动解析**: 编辑完整标识符时自动解析到各分段字段

#### 智能生成
- **一键生成**: 点击"生成"按钮自动填充所有字段
- **示例数据**: 自动生成符合字符集规范的示例组织机构代码
- **当前日期**: 自动填入当前日期作为生产日期
- **随机序列码**: 生成符合规范的随机序列码（含固定码S）
- **字符集限制**: 所有生成的字符均符合0~9、A~Z（不含O、I）的规范

#### 用户界面
- **深色主题**: 与应用整体风格保持一致
- **只读字段**: 校验码字段采用特殊样式表示只读状态
- **结果高亮**: 最终结果字段采用特殊样式突出显示
- **等宽字体**: 结果字段使用等宽字体便于查看

### 配置兼容性
- **向后兼容**: 支持加载旧版本的单一字段配置
- **自动转换**: 旧配置会自动解析为分段格式
- **新格式保存**: 新配置以分段格式保存，提供更好的可读性

## 注意事项

1. **网络权限**: 确保应用程序具有网络发送权限
2. **端口占用**: 避免目标端口被其他应用占用
3. **数据验证**: 输入数据会进行基本验证，但建议在使用前进行额外检查
4. **内存管理**: 类会自动管理内部资源，无需手动释放
5. **线程安全**: 该类设计为在主线程中使用，不保证线程安全
6. **唯一标识符格式**: 确保分段输入的字符数符合要求，系统会自动进行长度验证

## 扩展建议

1. **协议扩展**: 可通过修改 `buildDataFrame()` 方法支持其他协议格式
2. **网络协议**: 可扩展支持TCP等其他网络协议
3. **数据验证**: 可增加更严格的数据验证机制
4. **插件化**: 可将不同协议实现为插件形式

## 版本信息

- **创建日期**: 2024年
- **最后更新**: 2024年
- **版本**: 1.0
- **作者**: DataProcessor项目组

---

*本文档描述了 FaultAlarmWidget 类的完整规范，包括接口定义、使用方法和实现细节。如有疑问或建议，请联系项目维护人员。*