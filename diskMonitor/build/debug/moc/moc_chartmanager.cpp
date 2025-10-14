/****************************************************************************
** Meta object code from reading C++ file 'chartmanager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../chartmanager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'chartmanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ChartManager_t {
    QByteArrayData data[26];
    char stringdata0[350];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ChartManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ChartManager_t qt_meta_stringdata_ChartManager = {
    {
QT_MOC_LITERAL(0, 0, 12), // "ChartManager"
QT_MOC_LITERAL(1, 13, 12), // "chartCreated"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 9), // "ChartType"
QT_MOC_LITERAL(4, 37, 4), // "type"
QT_MOC_LITERAL(5, 42, 11), // "QChartView*"
QT_MOC_LITERAL(6, 54, 9), // "chartView"
QT_MOC_LITERAL(7, 64, 12), // "chartUpdated"
QT_MOC_LITERAL(8, 77, 13), // "chartExported"
QT_MOC_LITERAL(9, 91, 8), // "filePath"
QT_MOC_LITERAL(10, 100, 21), // "realTimeUpdateStarted"
QT_MOC_LITERAL(11, 122, 21), // "realTimeUpdateStopped"
QT_MOC_LITERAL(12, 144, 13), // "errorOccurred"
QT_MOC_LITERAL(13, 158, 5), // "error"
QT_MOC_LITERAL(14, 164, 16), // "refreshAllCharts"
QT_MOC_LITERAL(15, 181, 12), // "refreshChart"
QT_MOC_LITERAL(16, 194, 23), // "onDriveSelectionChanged"
QT_MOC_LITERAL(17, 218, 11), // "driveLetter"
QT_MOC_LITERAL(18, 230, 18), // "onTimeRangeChanged"
QT_MOC_LITERAL(19, 249, 9), // "TimeRange"
QT_MOC_LITERAL(20, 259, 5), // "range"
QT_MOC_LITERAL(21, 265, 20), // "onRealTimeDataUpdate"
QT_MOC_LITERAL(22, 286, 20), // "updateRealTimeCharts"
QT_MOC_LITERAL(23, 307, 21), // "onHistoryDataRecorded"
QT_MOC_LITERAL(24, 329, 13), // "HistoryRecord"
QT_MOC_LITERAL(25, 343, 6) // "record"

    },
    "ChartManager\0chartCreated\0\0ChartType\0"
    "type\0QChartView*\0chartView\0chartUpdated\0"
    "chartExported\0filePath\0realTimeUpdateStarted\0"
    "realTimeUpdateStopped\0errorOccurred\0"
    "error\0refreshAllCharts\0refreshChart\0"
    "onDriveSelectionChanged\0driveLetter\0"
    "onTimeRangeChanged\0TimeRange\0range\0"
    "onRealTimeDataUpdate\0updateRealTimeCharts\0"
    "onHistoryDataRecorded\0HistoryRecord\0"
    "record"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ChartManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   79,    2, 0x06 /* Public */,
       7,    1,   84,    2, 0x06 /* Public */,
       8,    1,   87,    2, 0x06 /* Public */,
      10,    0,   90,    2, 0x06 /* Public */,
      11,    0,   91,    2, 0x06 /* Public */,
      12,    1,   92,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      14,    0,   95,    2, 0x0a /* Public */,
      15,    1,   96,    2, 0x0a /* Public */,
      16,    1,   99,    2, 0x0a /* Public */,
      18,    1,  102,    2, 0x0a /* Public */,
      21,    0,  105,    2, 0x0a /* Public */,
      22,    0,  106,    2, 0x08 /* Private */,
      23,    1,  107,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   13,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 24,   25,

       0        // eod
};

void ChartManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChartManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->chartCreated((*reinterpret_cast< ChartType(*)>(_a[1])),(*reinterpret_cast< QChartView*(*)>(_a[2]))); break;
        case 1: _t->chartUpdated((*reinterpret_cast< ChartType(*)>(_a[1]))); break;
        case 2: _t->chartExported((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->realTimeUpdateStarted(); break;
        case 4: _t->realTimeUpdateStopped(); break;
        case 5: _t->errorOccurred((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->refreshAllCharts(); break;
        case 7: _t->refreshChart((*reinterpret_cast< ChartType(*)>(_a[1]))); break;
        case 8: _t->onDriveSelectionChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->onTimeRangeChanged((*reinterpret_cast< TimeRange(*)>(_a[1]))); break;
        case 10: _t->onRealTimeDataUpdate(); break;
        case 11: _t->updateRealTimeCharts(); break;
        case 12: _t->onHistoryDataRecorded((*reinterpret_cast< const HistoryRecord(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QChartView* >(); break;
            }
            break;
        case 12:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< HistoryRecord >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChartManager::*)(ChartType , QChartView * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartManager::chartCreated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ChartManager::*)(ChartType );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartManager::chartUpdated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ChartManager::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartManager::chartExported)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ChartManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartManager::realTimeUpdateStarted)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ChartManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartManager::realTimeUpdateStopped)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ChartManager::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartManager::errorOccurred)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ChartManager::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_ChartManager.data,
    qt_meta_data_ChartManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ChartManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChartManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChartManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ChartManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void ChartManager::chartCreated(ChartType _t1, QChartView * _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ChartManager::chartUpdated(ChartType _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ChartManager::chartExported(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ChartManager::realTimeUpdateStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ChartManager::realTimeUpdateStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ChartManager::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
