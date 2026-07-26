/****************************************************************************
** Meta object code from reading C++ file 'InputEngine.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/input/InputEngine.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'InputEngine.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN11InputEngineE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN11InputEngineE = QtMocHelpers::stringData(
    "InputEngine",
    "screenshotRequested",
    "",
    "replayRequested",
    "overlayToggleRequested",
    "overlayHideRequested",
    "overlayNavigate",
    "direction",
    "overlayNavigateVertical",
    "overlayConfirm",
    "overlayFavorite",
    "overlayMenu",
    "overlaySidebarToggle",
    "overlayGameStep",
    "desktopNavigate",
    "desktopNavigateVertical",
    "desktopConfirm",
    "desktopBack",
    "desktopFavorite",
    "desktopMenu",
    "desktopTabStep",
    "desktopSettings",
    "desktopZoom",
    "desktopBulkToggle",
    "playbackPlayPause",
    "playbackSeek",
    "frameGrabRequested",
    "lastInputChanged",
    "controllerStatusChanged",
    "controllerWarningChanged",
    "setOverlayVisible",
    "visible",
    "setDesktopFocused",
    "focused",
    "setPlaybackActive",
    "active",
    "reloadBindings",
    "handleKeyPressed",
    "key",
    "modifiers",
    "autoRepeat",
    "handleKeyReleased",
    "fixHiddenController",
    "lastInput",
    "controllerStatus",
    "controllerWarning",
    "controllerFixAvailable",
    "bindingEditor"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN11InputEngineE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      35,   14, // methods
       5,  295, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      27,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  224,    2, 0x06,    6 /* Public */,
       3,    0,  225,    2, 0x06,    7 /* Public */,
       4,    0,  226,    2, 0x06,    8 /* Public */,
       5,    0,  227,    2, 0x06,    9 /* Public */,
       6,    1,  228,    2, 0x06,   10 /* Public */,
       8,    1,  231,    2, 0x06,   12 /* Public */,
       9,    0,  234,    2, 0x06,   14 /* Public */,
      10,    0,  235,    2, 0x06,   15 /* Public */,
      11,    0,  236,    2, 0x06,   16 /* Public */,
      12,    0,  237,    2, 0x06,   17 /* Public */,
      13,    1,  238,    2, 0x06,   18 /* Public */,
      14,    1,  241,    2, 0x06,   20 /* Public */,
      15,    1,  244,    2, 0x06,   22 /* Public */,
      16,    0,  247,    2, 0x06,   24 /* Public */,
      17,    0,  248,    2, 0x06,   25 /* Public */,
      18,    0,  249,    2, 0x06,   26 /* Public */,
      19,    0,  250,    2, 0x06,   27 /* Public */,
      20,    1,  251,    2, 0x06,   28 /* Public */,
      21,    0,  254,    2, 0x06,   30 /* Public */,
      22,    1,  255,    2, 0x06,   31 /* Public */,
      23,    0,  258,    2, 0x06,   33 /* Public */,
      24,    0,  259,    2, 0x06,   34 /* Public */,
      25,    1,  260,    2, 0x06,   35 /* Public */,
      26,    0,  263,    2, 0x06,   37 /* Public */,
      27,    0,  264,    2, 0x06,   38 /* Public */,
      28,    0,  265,    2, 0x06,   39 /* Public */,
      29,    0,  266,    2, 0x06,   40 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      30,    1,  267,    2, 0x0a,   41 /* Public */,
      32,    1,  270,    2, 0x0a,   43 /* Public */,
      34,    1,  273,    2, 0x0a,   45 /* Public */,
      36,    0,  276,    2, 0x0a,   47 /* Public */,
      37,    3,  277,    2, 0x0a,   48 /* Public */,
      37,    2,  284,    2, 0x2a,   52 /* Public | MethodCloned */,
      41,    2,  289,    2, 0x0a,   55 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      42,    0,  294,    2, 0x02,   58 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,   31,
    QMetaType::Void, QMetaType::Bool,   33,
    QMetaType::Void, QMetaType::Bool,   35,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::Int, QMetaType::Int, QMetaType::Bool,   38,   39,   40,
    QMetaType::Bool, QMetaType::Int, QMetaType::Int,   38,   39,
    QMetaType::Bool, QMetaType::Int, QMetaType::Int,   38,   39,

 // methods: parameters
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      43, QMetaType::QString, 0x00015001, uint(24), 0,
      44, QMetaType::QString, 0x00015001, uint(25), 0,
      45, QMetaType::QString, 0x00015001, uint(26), 0,
      46, QMetaType::Bool, 0x00015001, uint(26), 0,
      47, QMetaType::QObjectStar, 0x00015401, uint(-1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject InputEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN11InputEngineE.offsetsAndSizes,
    qt_meta_data_ZN11InputEngineE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN11InputEngineE_t,
        // property 'lastInput'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'controllerStatus'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'controllerWarning'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'controllerFixAvailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'bindingEditor'
        QtPrivate::TypeAndForceComplete<QObject*, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<InputEngine, std::true_type>,
        // method 'screenshotRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'replayRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlayToggleRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlayHideRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlayNavigate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'overlayNavigateVertical'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'overlayConfirm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlayFavorite'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlayMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlaySidebarToggle'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlayGameStep'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'desktopNavigate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'desktopNavigateVertical'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'desktopConfirm'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'desktopBack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'desktopFavorite'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'desktopMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'desktopTabStep'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'desktopSettings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'desktopZoom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'desktopBulkToggle'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'playbackPlayPause'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'playbackSeek'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'frameGrabRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastInputChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'controllerStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'controllerWarningChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setOverlayVisible'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setDesktopFocused'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setPlaybackActive'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'reloadBindings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleKeyPressed'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'handleKeyPressed'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'handleKeyReleased'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'fixHiddenController'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void InputEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<InputEngine *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->screenshotRequested(); break;
        case 1: _t->replayRequested(); break;
        case 2: _t->overlayToggleRequested(); break;
        case 3: _t->overlayHideRequested(); break;
        case 4: _t->overlayNavigate((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->overlayNavigateVertical((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->overlayConfirm(); break;
        case 7: _t->overlayFavorite(); break;
        case 8: _t->overlayMenu(); break;
        case 9: _t->overlaySidebarToggle(); break;
        case 10: _t->overlayGameStep((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->desktopNavigate((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->desktopNavigateVertical((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 13: _t->desktopConfirm(); break;
        case 14: _t->desktopBack(); break;
        case 15: _t->desktopFavorite(); break;
        case 16: _t->desktopMenu(); break;
        case 17: _t->desktopTabStep((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 18: _t->desktopSettings(); break;
        case 19: _t->desktopZoom((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 20: _t->desktopBulkToggle(); break;
        case 21: _t->playbackPlayPause(); break;
        case 22: _t->playbackSeek((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 23: _t->frameGrabRequested(); break;
        case 24: _t->lastInputChanged(); break;
        case 25: _t->controllerStatusChanged(); break;
        case 26: _t->controllerWarningChanged(); break;
        case 27: _t->setOverlayVisible((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 28: _t->setDesktopFocused((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 29: _t->setPlaybackActive((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 30: _t->reloadBindings(); break;
        case 31: { bool _r = _t->handleKeyPressed((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 32: { bool _r = _t->handleKeyPressed((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 33: { bool _r = _t->handleKeyReleased((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 34: _t->fixHiddenController(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::screenshotRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::replayRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::overlayToggleRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::overlayHideRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)(int );
            if (_q_method_type _q_method = &InputEngine::overlayNavigate; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)(int );
            if (_q_method_type _q_method = &InputEngine::overlayNavigateVertical; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::overlayConfirm; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::overlayFavorite; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::overlayMenu; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::overlaySidebarToggle; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)(int );
            if (_q_method_type _q_method = &InputEngine::overlayGameStep; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)(int );
            if (_q_method_type _q_method = &InputEngine::desktopNavigate; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)(int );
            if (_q_method_type _q_method = &InputEngine::desktopNavigateVertical; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::desktopConfirm; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::desktopBack; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::desktopFavorite; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::desktopMenu; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)(int );
            if (_q_method_type _q_method = &InputEngine::desktopTabStep; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 17;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::desktopSettings; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 18;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)(int );
            if (_q_method_type _q_method = &InputEngine::desktopZoom; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 19;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::desktopBulkToggle; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 20;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::playbackPlayPause; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 21;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)(int );
            if (_q_method_type _q_method = &InputEngine::playbackSeek; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 22;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::frameGrabRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 23;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::lastInputChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 24;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::controllerStatusChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 25;
                return;
            }
        }
        {
            using _q_method_type = void (InputEngine::*)();
            if (_q_method_type _q_method = &InputEngine::controllerWarningChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 26;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->lastInput(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->controllerStatus(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->controllerWarning(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->controllerFixAvailable(); break;
        case 4: *reinterpret_cast< QObject**>(_v) = _t->bindingEditor(); break;
        default: break;
        }
    }
}

const QMetaObject *InputEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *InputEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN11InputEngineE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int InputEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 35)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 35;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 35)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 35;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void InputEngine::screenshotRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void InputEngine::replayRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void InputEngine::overlayToggleRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void InputEngine::overlayHideRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void InputEngine::overlayNavigate(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void InputEngine::overlayNavigateVertical(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void InputEngine::overlayConfirm()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void InputEngine::overlayFavorite()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void InputEngine::overlayMenu()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void InputEngine::overlaySidebarToggle()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void InputEngine::overlayGameStep(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void InputEngine::desktopNavigate(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void InputEngine::desktopNavigateVertical(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void InputEngine::desktopConfirm()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void InputEngine::desktopBack()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void InputEngine::desktopFavorite()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void InputEngine::desktopMenu()
{
    QMetaObject::activate(this, &staticMetaObject, 16, nullptr);
}

// SIGNAL 17
void InputEngine::desktopTabStep(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void InputEngine::desktopSettings()
{
    QMetaObject::activate(this, &staticMetaObject, 18, nullptr);
}

// SIGNAL 19
void InputEngine::desktopZoom(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 19, _a);
}

// SIGNAL 20
void InputEngine::desktopBulkToggle()
{
    QMetaObject::activate(this, &staticMetaObject, 20, nullptr);
}

// SIGNAL 21
void InputEngine::playbackPlayPause()
{
    QMetaObject::activate(this, &staticMetaObject, 21, nullptr);
}

// SIGNAL 22
void InputEngine::playbackSeek(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 22, _a);
}

// SIGNAL 23
void InputEngine::frameGrabRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 23, nullptr);
}

// SIGNAL 24
void InputEngine::lastInputChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 24, nullptr);
}

// SIGNAL 25
void InputEngine::controllerStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 25, nullptr);
}

// SIGNAL 26
void InputEngine::controllerWarningChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 26, nullptr);
}
QT_WARNING_POP
