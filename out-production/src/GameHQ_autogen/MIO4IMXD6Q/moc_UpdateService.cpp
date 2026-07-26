/****************************************************************************
** Meta object code from reading C++ file 'UpdateService.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/updates/UpdateService.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'UpdateService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN13UpdateServiceE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13UpdateServiceE = QtMocHelpers::stringData(
    "UpdateService",
    "stateChanged",
    "",
    "releaseChanged",
    "progressChanged",
    "errorChanged",
    "lastCheckedChanged",
    "versionSkipped",
    "version",
    "etagUpdated",
    "etag",
    "nextAllowedCheckChanged",
    "when",
    "prepareForUpdateRequested",
    "quiescenceReached",
    "installApproved",
    "VerifiedUpdate",
    "verified",
    "checkNow",
    "downloadUpdate",
    "cancelDownload",
    "installAndRestart",
    "skipVersion",
    "openReleasePage",
    "state",
    "State",
    "stateName",
    "installedVersion",
    "latestVersion",
    "releaseName",
    "notes",
    "releaseUrl",
    "size",
    "publishedAt",
    "progress",
    "errorText",
    "lastChecked",
    "failedDuringCheck",
    "Idle",
    "Checking",
    "UpToDate",
    "UpdateAvailable",
    "Downloading",
    "ReadyToInstall",
    "PreparingForUpdate",
    "Quiescent",
    "Installing",
    "Failed"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13UpdateServiceE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
      13,  141, // properties
       1,  206, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  116,    2, 0x06,   15 /* Public */,
       3,    0,  117,    2, 0x06,   16 /* Public */,
       4,    0,  118,    2, 0x06,   17 /* Public */,
       5,    0,  119,    2, 0x06,   18 /* Public */,
       6,    0,  120,    2, 0x06,   19 /* Public */,
       7,    1,  121,    2, 0x06,   20 /* Public */,
       9,    1,  124,    2, 0x06,   22 /* Public */,
      11,    1,  127,    2, 0x06,   24 /* Public */,
      13,    0,  130,    2, 0x06,   26 /* Public */,
      14,    0,  131,    2, 0x06,   27 /* Public */,
      15,    1,  132,    2, 0x06,   28 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      18,    0,  135,    2, 0x02,   30 /* Public */,
      19,    0,  136,    2, 0x02,   31 /* Public */,
      20,    0,  137,    2, 0x02,   32 /* Public */,
      21,    0,  138,    2, 0x02,   33 /* Public */,
      22,    0,  139,    2, 0x02,   34 /* Public */,
      23,    0,  140,    2, 0x102,   35 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QDateTime,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 16,   17,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      24, 0x80000000 | 25, 0x00015009, uint(0), 0,
      26, QMetaType::QString, 0x00015001, uint(0), 0,
      27, QMetaType::QString, 0x00015401, uint(-1), 0,
      28, QMetaType::QString, 0x00015001, uint(1), 0,
      29, QMetaType::QString, 0x00015001, uint(1), 0,
      30, QMetaType::QString, 0x00015001, uint(1), 0,
      31, QMetaType::QString, 0x00015001, uint(1), 0,
      32, QMetaType::LongLong, 0x00015001, uint(1), 0,
      33, QMetaType::QDateTime, 0x00015001, uint(1), 0,
      34, QMetaType::Int, 0x00015001, uint(2), 0,
      35, QMetaType::QString, 0x00015001, uint(3), 0,
      36, QMetaType::QDateTime, 0x00015001, uint(4), 0,
      37, QMetaType::Bool, 0x00015001, uint(0), 0,

 // enums: name, alias, flags, count, data
      25,   25, 0x2,   10,  211,

 // enum data: key, value
      38, uint(UpdateService::State::Idle),
      39, uint(UpdateService::State::Checking),
      40, uint(UpdateService::State::UpToDate),
      41, uint(UpdateService::State::UpdateAvailable),
      42, uint(UpdateService::State::Downloading),
      43, uint(UpdateService::State::ReadyToInstall),
      44, uint(UpdateService::State::PreparingForUpdate),
      45, uint(UpdateService::State::Quiescent),
      46, uint(UpdateService::State::Installing),
      47, uint(UpdateService::State::Failed),

       0        // eod
};

Q_CONSTINIT const QMetaObject UpdateService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN13UpdateServiceE.offsetsAndSizes,
    qt_meta_data_ZN13UpdateServiceE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13UpdateServiceE_t,
        // property 'state'
        QtPrivate::TypeAndForceComplete<State, std::true_type>,
        // property 'stateName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'installedVersion'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'latestVersion'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'releaseName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'notes'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'releaseUrl'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'size'
        QtPrivate::TypeAndForceComplete<qint64, std::true_type>,
        // property 'publishedAt'
        QtPrivate::TypeAndForceComplete<QDateTime, std::true_type>,
        // property 'progress'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'errorText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'lastChecked'
        QtPrivate::TypeAndForceComplete<QDateTime, std::true_type>,
        // property 'failedDuringCheck'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // enum 'State'
        QtPrivate::TypeAndForceComplete<UpdateService::State, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UpdateService, std::true_type>,
        // method 'stateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'releaseChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'progressChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'errorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastCheckedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'versionSkipped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'etagUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'nextAllowedCheckChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QDateTime &, std::false_type>,
        // method 'prepareForUpdateRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'quiescenceReached'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'installApproved'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const VerifiedUpdate &, std::false_type>,
        // method 'checkNow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'downloadUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cancelDownload'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'installAndRestart'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'skipVersion'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openReleasePage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void UpdateService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<UpdateService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->stateChanged(); break;
        case 1: _t->releaseChanged(); break;
        case 2: _t->progressChanged(); break;
        case 3: _t->errorChanged(); break;
        case 4: _t->lastCheckedChanged(); break;
        case 5: _t->versionSkipped((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->etagUpdated((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->nextAllowedCheckChanged((*reinterpret_cast< std::add_pointer_t<QDateTime>>(_a[1]))); break;
        case 8: _t->prepareForUpdateRequested(); break;
        case 9: _t->quiescenceReached(); break;
        case 10: _t->installApproved((*reinterpret_cast< std::add_pointer_t<VerifiedUpdate>>(_a[1]))); break;
        case 11: _t->checkNow(); break;
        case 12: _t->downloadUpdate(); break;
        case 13: _t->cancelDownload(); break;
        case 14: _t->installAndRestart(); break;
        case 15: _t->skipVersion(); break;
        case 16: _t->openReleasePage(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (UpdateService::*)();
            if (_q_method_type _q_method = &UpdateService::stateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)();
            if (_q_method_type _q_method = &UpdateService::releaseChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)();
            if (_q_method_type _q_method = &UpdateService::progressChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)();
            if (_q_method_type _q_method = &UpdateService::errorChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)();
            if (_q_method_type _q_method = &UpdateService::lastCheckedChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)(const QString & );
            if (_q_method_type _q_method = &UpdateService::versionSkipped; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)(const QString & );
            if (_q_method_type _q_method = &UpdateService::etagUpdated; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)(const QDateTime & );
            if (_q_method_type _q_method = &UpdateService::nextAllowedCheckChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)();
            if (_q_method_type _q_method = &UpdateService::prepareForUpdateRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)();
            if (_q_method_type _q_method = &UpdateService::quiescenceReached; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (UpdateService::*)(const VerifiedUpdate & );
            if (_q_method_type _q_method = &UpdateService::installApproved; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< State*>(_v) = _t->state(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->stateName(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->installedVersion(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->latestVersion(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->releaseName(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->notes(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->releaseUrl(); break;
        case 7: *reinterpret_cast< qint64*>(_v) = _t->size(); break;
        case 8: *reinterpret_cast< QDateTime*>(_v) = _t->publishedAt(); break;
        case 9: *reinterpret_cast< int*>(_v) = _t->progress(); break;
        case 10: *reinterpret_cast< QString*>(_v) = _t->errorText(); break;
        case 11: *reinterpret_cast< QDateTime*>(_v) = _t->lastChecked(); break;
        case 12: *reinterpret_cast< bool*>(_v) = _t->failedDuringCheck(); break;
        default: break;
        }
    }
}

const QMetaObject *UpdateService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UpdateService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13UpdateServiceE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UpdateService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 17;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void UpdateService::stateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void UpdateService::releaseChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void UpdateService::progressChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void UpdateService::errorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void UpdateService::lastCheckedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void UpdateService::versionSkipped(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void UpdateService::etagUpdated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void UpdateService::nextAllowedCheckChanged(const QDateTime & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void UpdateService::prepareForUpdateRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void UpdateService::quiescenceReached()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void UpdateService::installApproved(const VerifiedUpdate & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}
QT_WARNING_POP
