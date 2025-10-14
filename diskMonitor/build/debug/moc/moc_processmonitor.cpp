/****************************************************************************
** Meta object code from reading C++ file 'processmonitor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../processmonitor.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'processmonitor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ProcessMonitor_t {
    QByteArrayData data[24];
    char stringdata0[310];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ProcessMonitor_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ProcessMonitor_t qt_meta_stringdata_ProcessMonitor = {
    {
QT_MOC_LITERAL(0, 0, 14), // "ProcessMonitor"
QT_MOC_LITERAL(1, 15, 18), // "processDataUpdated"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 17), // "driveStatsUpdated"
QT_MOC_LITERAL(4, 53, 5), // "drive"
QT_MOC_LITERAL(5, 59, 17), // "DriveProcessStats"
QT_MOC_LITERAL(6, 77, 5), // "stats"
QT_MOC_LITERAL(7, 83, 13), // "errorOccurred"
QT_MOC_LITERAL(8, 97, 5), // "error"
QT_MOC_LITERAL(9, 103, 14), // "processStarted"
QT_MOC_LITERAL(10, 118, 11), // "ProcessInfo"
QT_MOC_LITERAL(11, 130, 7), // "process"
QT_MOC_LITERAL(12, 138, 17), // "processTerminated"
QT_MOC_LITERAL(13, 156, 9), // "processId"
QT_MOC_LITERAL(14, 166, 18), // "refreshProcessData"
QT_MOC_LITERAL(15, 185, 17), // "updateDriveFilter"
QT_MOC_LITERAL(16, 203, 6), // "drives"
QT_MOC_LITERAL(17, 210, 14), // "onTimerTimeout"
QT_MOC_LITERAL(18, 225, 17), // "requestDriveStats"
QT_MOC_LITERAL(19, 243, 11), // "driveLetter"
QT_MOC_LITERAL(20, 255, 15), // "startMonitoring"
QT_MOC_LITERAL(21, 271, 10), // "intervalMs"
QT_MOC_LITERAL(22, 282, 14), // "setDriveFilter"
QT_MOC_LITERAL(23, 297, 12) // "driveLetters"

    },
    "ProcessMonitor\0processDataUpdated\0\0"
    "driveStatsUpdated\0drive\0DriveProcessStats\0"
    "stats\0errorOccurred\0error\0processStarted\0"
    "ProcessInfo\0process\0processTerminated\0"
    "processId\0refreshProcessData\0"
    "updateDriveFilter\0drives\0onTimerTimeout\0"
    "requestDriveStats\0driveLetter\0"
    "startMonitoring\0intervalMs\0setDriveFilter\0"
    "driveLetters"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ProcessMonitor[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   74,    2, 0x06 /* Public */,
       3,    2,   75,    2, 0x06 /* Public */,
       7,    1,   80,    2, 0x06 /* Public */,
       9,    1,   83,    2, 0x06 /* Public */,
      12,    1,   86,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      14,    0,   89,    2, 0x0a /* Public */,
      15,    1,   90,    2, 0x0a /* Public */,
      17,    0,   93,    2, 0x0a /* Public */,
      18,    1,   94,    2, 0x0a /* Public */,

 // methods: name, argc, parameters, tag, flags
      20,    1,   97,    2, 0x02 /* Public */,
      20,    0,  100,    2, 0x22 /* Public | MethodCloned */,
      22,    1,  101,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 5,    4,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void, QMetaType::UInt,   13,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QStringList,   16,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   19,

 // methods: parameters
    QMetaType::Void, QMetaType::Int,   21,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QStringList,   23,

       0        // eod
};

void ProcessMonitor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ProcessMonitor *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->processDataUpdated(); break;
        case 1: _t->driveStatsUpdated((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const DriveProcessStats(*)>(_a[2]))); break;
        case 2: _t->errorOccurred((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->processStarted((*reinterpret_cast< const ProcessInfo(*)>(_a[1]))); break;
        case 4: _t->processTerminated((*reinterpret_cast< quint32(*)>(_a[1]))); break;
        case 5: _t->refreshProcessData(); break;
        case 6: _t->updateDriveFilter((*reinterpret_cast< const QStringList(*)>(_a[1]))); break;
        case 7: _t->onTimerTimeout(); break;
        case 8: _t->requestDriveStats((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->startMonitoring((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->startMonitoring(); break;
        case 11: _t->setDriveFilter((*reinterpret_cast< const QStringList(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ProcessMonitor::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProcessMonitor::processDataUpdated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ProcessMonitor::*)(const QString & , const DriveProcessStats & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProcessMonitor::driveStatsUpdated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ProcessMonitor::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProcessMonitor::errorOccurred)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ProcessMonitor::*)(const ProcessInfo & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProcessMonitor::processStarted)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ProcessMonitor::*)(quint32 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProcessMonitor::processTerminated)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ProcessMonitor::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_ProcessMonitor.data,
    qt_meta_data_ProcessMonitor,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ProcessMonitor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ProcessMonitor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ProcessMonitor.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ProcessMonitor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void ProcessMonitor::processDataUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ProcessMonitor::driveStatsUpdated(const QString & _t1, const DriveProcessStats & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ProcessMonitor::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ProcessMonitor::processStarted(const ProcessInfo & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ProcessMonitor::processTerminated(quint32 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
