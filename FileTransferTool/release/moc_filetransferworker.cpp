/****************************************************************************
** Meta object code from reading C++ file 'filetransferworker.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../filetransferworker.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'filetransferworker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FileTransferWorker_t {
    QByteArrayData data[10];
    char stringdata0[106];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FileTransferWorker_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FileTransferWorker_t qt_meta_stringdata_FileTransferWorker = {
    {
QT_MOC_LITERAL(0, 0, 18), // "FileTransferWorker"
QT_MOC_LITERAL(1, 19, 15), // "progressUpdated"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 7), // "current"
QT_MOC_LITERAL(4, 44, 5), // "total"
QT_MOC_LITERAL(5, 50, 8), // "fileName"
QT_MOC_LITERAL(6, 59, 16), // "transferFinished"
QT_MOC_LITERAL(7, 76, 7), // "success"
QT_MOC_LITERAL(8, 84, 7), // "message"
QT_MOC_LITERAL(9, 92, 13) // "startTransfer"

    },
    "FileTransferWorker\0progressUpdated\0\0"
    "current\0total\0fileName\0transferFinished\0"
    "success\0message\0startTransfer"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FileTransferWorker[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    3,   29,    2, 0x06 /* Public */,
       6,    2,   36,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    0,   41,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,    3,    4,    5,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void FileTransferWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FileTransferWorker *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->progressUpdated((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 1: _t->transferFinished((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 2: _t->startTransfer(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FileTransferWorker::*)(int , int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FileTransferWorker::progressUpdated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FileTransferWorker::*)(bool , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FileTransferWorker::transferFinished)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject FileTransferWorker::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_FileTransferWorker.data,
    qt_meta_data_FileTransferWorker,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *FileTransferWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FileTransferWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FileTransferWorker.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int FileTransferWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void FileTransferWorker::progressUpdated(int _t1, int _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FileTransferWorker::transferFinished(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
