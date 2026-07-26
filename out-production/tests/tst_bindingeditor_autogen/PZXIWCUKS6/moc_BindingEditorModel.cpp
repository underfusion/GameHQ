/****************************************************************************
** Meta object code from reading C++ file 'BindingEditorModel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/input/BindingEditorModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BindingEditorModel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN18BindingEditorModelE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN18BindingEditorModelE = QtMocHelpers::stringData(
    "BindingEditorModel",
    "deviceGroupChanged",
    "",
    "rowsChanged",
    "controllerSpecificChanged",
    "controllerProfileChanged",
    "captureChanged",
    "conflictChanged",
    "lastFiredActionChanged",
    "beginCapture",
    "actionId",
    "slot",
    "cancelCapture",
    "clearBinding",
    "resetAction",
    "resetCurrentProfile",
    "resetAllBindings",
    "confirmConflict",
    "dismissConflict",
    "deviceGroup",
    "rows",
    "QVariantList",
    "controllerSpecific",
    "controllerSpecificAvailable",
    "controllerName",
    "captureActive",
    "capturePrompt",
    "conflictPending",
    "conflictMessage",
    "lastFiredAction"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN18BindingEditorModelE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
      10,  129, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  104,    2, 0x06,   11 /* Public */,
       3,    0,  105,    2, 0x06,   12 /* Public */,
       4,    0,  106,    2, 0x06,   13 /* Public */,
       5,    0,  107,    2, 0x06,   14 /* Public */,
       6,    0,  108,    2, 0x06,   15 /* Public */,
       7,    0,  109,    2, 0x06,   16 /* Public */,
       8,    0,  110,    2, 0x06,   17 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       9,    2,  111,    2, 0x02,   18 /* Public */,
      12,    0,  116,    2, 0x02,   21 /* Public */,
      13,    2,  117,    2, 0x02,   22 /* Public */,
      14,    1,  122,    2, 0x02,   25 /* Public */,
      15,    0,  125,    2, 0x02,   27 /* Public */,
      16,    0,  126,    2, 0x02,   28 /* Public */,
      17,    0,  127,    2, 0x02,   29 /* Public */,
      18,    0,  128,    2, 0x02,   30 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   10,   11,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   10,   11,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      19, QMetaType::QString, 0x00015103, uint(0), 0,
      20, 0x80000000 | 21, 0x00015009, uint(1), 0,
      22, QMetaType::Bool, 0x00015103, uint(2), 0,
      23, QMetaType::Bool, 0x00015001, uint(3), 0,
      24, QMetaType::QString, 0x00015001, uint(3), 0,
      25, QMetaType::Bool, 0x00015001, uint(4), 0,
      26, QMetaType::QString, 0x00015001, uint(4), 0,
      27, QMetaType::Bool, 0x00015001, uint(5), 0,
      28, QMetaType::QString, 0x00015001, uint(5), 0,
      29, QMetaType::QString, 0x00015001, uint(6), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject BindingEditorModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN18BindingEditorModelE.offsetsAndSizes,
    qt_meta_data_ZN18BindingEditorModelE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN18BindingEditorModelE_t,
        // property 'deviceGroup'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'rows'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'controllerSpecific'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'controllerSpecificAvailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'controllerName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'captureActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'capturePrompt'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'conflictPending'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'conflictMessage'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'lastFiredAction'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<BindingEditorModel, std::true_type>,
        // method 'deviceGroupChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'rowsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'controllerSpecificChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'controllerProfileChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'captureChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'conflictChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastFiredActionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'beginCapture'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'cancelCapture'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearBinding'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'resetAction'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resetCurrentProfile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetAllBindings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'confirmConflict'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'dismissConflict'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void BindingEditorModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<BindingEditorModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->deviceGroupChanged(); break;
        case 1: _t->rowsChanged(); break;
        case 2: _t->controllerSpecificChanged(); break;
        case 3: _t->controllerProfileChanged(); break;
        case 4: _t->captureChanged(); break;
        case 5: _t->conflictChanged(); break;
        case 6: _t->lastFiredActionChanged(); break;
        case 7: _t->beginCapture((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 8: _t->cancelCapture(); break;
        case 9: _t->clearBinding((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 10: _t->resetAction((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->resetCurrentProfile(); break;
        case 12: _t->resetAllBindings(); break;
        case 13: _t->confirmConflict(); break;
        case 14: _t->dismissConflict(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (BindingEditorModel::*)();
            if (_q_method_type _q_method = &BindingEditorModel::deviceGroupChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (BindingEditorModel::*)();
            if (_q_method_type _q_method = &BindingEditorModel::rowsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (BindingEditorModel::*)();
            if (_q_method_type _q_method = &BindingEditorModel::controllerSpecificChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (BindingEditorModel::*)();
            if (_q_method_type _q_method = &BindingEditorModel::controllerProfileChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (BindingEditorModel::*)();
            if (_q_method_type _q_method = &BindingEditorModel::captureChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (BindingEditorModel::*)();
            if (_q_method_type _q_method = &BindingEditorModel::conflictChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (BindingEditorModel::*)();
            if (_q_method_type _q_method = &BindingEditorModel::lastFiredActionChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->deviceGroup(); break;
        case 1: *reinterpret_cast< QVariantList*>(_v) = _t->rows(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->controllerSpecific(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->controllerSpecificAvailable(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->controllerName(); break;
        case 5: *reinterpret_cast< bool*>(_v) = _t->captureActive(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->capturePrompt(); break;
        case 7: *reinterpret_cast< bool*>(_v) = _t->conflictPending(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->conflictMessage(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->lastFiredAction(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setDeviceGroup(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setControllerSpecific(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *BindingEditorModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BindingEditorModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN18BindingEditorModelE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int BindingEditorModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void BindingEditorModel::deviceGroupChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void BindingEditorModel::rowsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void BindingEditorModel::controllerSpecificChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void BindingEditorModel::controllerProfileChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void BindingEditorModel::captureChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void BindingEditorModel::conflictChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void BindingEditorModel::lastFiredActionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
