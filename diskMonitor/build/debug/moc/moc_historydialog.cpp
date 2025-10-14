/****************************************************************************
** Meta object code from reading C++ file 'historydialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../historydialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'historydialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_HistoryDialog_t {
    QByteArrayData data[11];
    char stringdata0[161];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_HistoryDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_HistoryDialog_t qt_meta_stringdata_HistoryDialog = {
    {
QT_MOC_LITERAL(0, 0, 13), // "HistoryDialog"
QT_MOC_LITERAL(1, 14, 13), // "onRefreshData"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 14), // "performRefresh"
QT_MOC_LITERAL(4, 44, 11), // "onExportCSV"
QT_MOC_LITERAL(5, 56, 13), // "onExportExcel"
QT_MOC_LITERAL(6, 70, 23), // "onDriveSelectionChanged"
QT_MOC_LITERAL(7, 94, 18), // "onTimeRangeChanged"
QT_MOC_LITERAL(8, 113, 20), // "onAutoRefreshToggled"
QT_MOC_LITERAL(9, 134, 7), // "enabled"
QT_MOC_LITERAL(10, 142, 18) // "onAutoRefreshTimer"

    },
    "HistoryDialog\0onRefreshData\0\0"
    "performRefresh\0onExportCSV\0onExportExcel\0"
    "onDriveSelectionChanged\0onTimeRangeChanged\0"
    "onAutoRefreshToggled\0enabled\0"
    "onAutoRefreshTimer"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_HistoryDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x08 /* Private */,
       3,    0,   55,    2, 0x08 /* Private */,
       4,    0,   56,    2, 0x08 /* Private */,
       5,    0,   57,    2, 0x08 /* Private */,
       6,    0,   58,    2, 0x08 /* Private */,
       7,    0,   59,    2, 0x08 /* Private */,
       8,    1,   60,    2, 0x08 /* Private */,
      10,    0,   63,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,

       0        // eod
};

void HistoryDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<HistoryDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onRefreshData(); break;
        case 1: _t->performRefresh(); break;
        case 2: _t->onExportCSV(); break;
        case 3: _t->onExportExcel(); break;
        case 4: _t->onDriveSelectionChanged(); break;
        case 5: _t->onTimeRangeChanged(); break;
        case 6: _t->onAutoRefreshToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->onAutoRefreshTimer(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject HistoryDialog::staticMetaObject = { {
    &QDialog::staticMetaObject,
    qt_meta_stringdata_HistoryDialog.data,
    qt_meta_data_HistoryDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *HistoryDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *HistoryDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_HistoryDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int HistoryDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
