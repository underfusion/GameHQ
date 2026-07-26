/****************************************************************************
** Meta object code from reading C++ file 'AppController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/ui/AppController.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AppController.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13AppControllerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN13AppControllerE = QtMocHelpers::stringData(
    "AppController",
    "gamesChanged",
    "",
    "filterChanged",
    "currentGameChanged",
    "overlayGameChanged",
    "foldersChanged",
    "captureLocationsChanged",
    "configChanged",
    "key",
    "QVariant",
    "value",
    "configGroupReset",
    "prefix",
    "replaySettingsChanged",
    "replayBufferStateChanged",
    "lastScanChanged",
    "hdrStatusChanged",
    "hdrDisplayConfigurationChanged",
    "refreshHdrStatus",
    "setCategory",
    "category",
    "setGame",
    "gameId",
    "setGameCategory",
    "rescan",
    "toggleFavorite",
    "row",
    "deleteCapture",
    "deleteCaptures",
    "QVariantList",
    "rows",
    "openCapture",
    "showInFolder",
    "deleteCaptureFrom",
    "GalleryModel*",
    "model",
    "openCaptureFrom",
    "showInFolderFrom",
    "syncOverlayToForegroundGame",
    "saveVideoFrame",
    "videoSink",
    "gameName",
    "executablePath",
    "addWatchedFolder",
    "folderUrl",
    "removeWatchedFolder",
    "path",
    "setCaptureRoot",
    "kind",
    "resetCaptureRoot",
    "openDataFolder",
    "openLogsFolder",
    "beginPortableImport",
    "quitApplication",
    "copyDiagnosticSummary",
    "resetCategory",
    "config",
    "fallback",
    "setConfig",
    "configDefault",
    "configIsDefault",
    "resetConfig",
    "resetConfigGroup",
    "resetAllConfig",
    "gallery",
    "version",
    "releaseNotesVersion",
    "releaseNotesSections",
    "games",
    "currentGameId",
    "currentGameAvailable",
    "overlayGameId",
    "watchedFolders",
    "capturesRoot",
    "screenshotsRoot",
    "clipsRoot",
    "managedRoots",
    "lastScanAdded",
    "lastScanAvailable",
    "startMinimized",
    "portableMode",
    "dataRoot",
    "logsRoot",
    "replayBufferActive",
    "replayBufferGame",
    "hdrDisplayActive",
    "hdrStatusText",
    "hdrDetailText"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN13AppControllerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      47,   14, // methods
      26,  423, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  296,    2, 0x06,   27 /* Public */,
       3,    0,  297,    2, 0x06,   28 /* Public */,
       4,    0,  298,    2, 0x06,   29 /* Public */,
       5,    0,  299,    2, 0x06,   30 /* Public */,
       6,    0,  300,    2, 0x06,   31 /* Public */,
       7,    0,  301,    2, 0x06,   32 /* Public */,
       8,    2,  302,    2, 0x06,   33 /* Public */,
      12,    1,  307,    2, 0x06,   36 /* Public */,
      14,    0,  310,    2, 0x06,   38 /* Public */,
      15,    0,  311,    2, 0x06,   39 /* Public */,
      16,    0,  312,    2, 0x06,   40 /* Public */,
      17,    0,  313,    2, 0x06,   41 /* Public */,
      18,    0,  314,    2, 0x06,   42 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      19,    0,  315,    2, 0x02,   43 /* Public */,
      20,    1,  316,    2, 0x02,   44 /* Public */,
      22,    1,  319,    2, 0x02,   46 /* Public */,
      24,    2,  322,    2, 0x02,   48 /* Public */,
      25,    0,  327,    2, 0x02,   51 /* Public */,
      26,    1,  328,    2, 0x02,   52 /* Public */,
      28,    1,  331,    2, 0x02,   54 /* Public */,
      29,    1,  334,    2, 0x02,   56 /* Public */,
      32,    1,  337,    2, 0x02,   58 /* Public */,
      33,    1,  340,    2, 0x02,   60 /* Public */,
      34,    2,  343,    2, 0x02,   62 /* Public */,
      37,    2,  348,    2, 0x02,   65 /* Public */,
      38,    2,  353,    2, 0x02,   68 /* Public */,
      39,    0,  358,    2, 0x02,   71 /* Public */,
      40,    3,  359,    2, 0x02,   72 /* Public */,
      40,    2,  366,    2, 0x22,   76 /* Public | MethodCloned */,
      44,    1,  371,    2, 0x02,   79 /* Public */,
      46,    1,  374,    2, 0x02,   81 /* Public */,
      48,    2,  377,    2, 0x02,   83 /* Public */,
      50,    1,  382,    2, 0x02,   86 /* Public */,
      51,    0,  385,    2, 0x02,   88 /* Public */,
      52,    0,  386,    2, 0x02,   89 /* Public */,
      53,    1,  387,    2, 0x02,   90 /* Public */,
      54,    0,  390,    2, 0x02,   92 /* Public */,
      55,    0,  391,    2, 0x102,   93 /* Public | MethodIsConst  */,
      56,    1,  392,    2, 0x02,   94 /* Public */,
      57,    2,  395,    2, 0x102,   96 /* Public | MethodIsConst  */,
      59,    2,  400,    2, 0x02,   99 /* Public */,
      60,    2,  405,    2, 0x102,  102 /* Public | MethodIsConst  */,
      60,    1,  410,    2, 0x122,  105 /* Public | MethodCloned | MethodIsConst  */,
      61,    1,  413,    2, 0x102,  107 /* Public | MethodIsConst  */,
      62,    1,  416,    2, 0x02,  109 /* Public */,
      63,    1,  419,    2, 0x02,  111 /* Public */,
      64,    0,  422,    2, 0x02,  113 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 10,    9,   11,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   21,
    QMetaType::Void, QMetaType::Int,   23,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   21,   23,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   27,
    QMetaType::Void, QMetaType::Int,   27,
    QMetaType::Void, 0x80000000 | 30,   31,
    QMetaType::Void, QMetaType::Int,   27,
    QMetaType::Void, QMetaType::Int,   27,
    QMetaType::Void, 0x80000000 | 35, QMetaType::Int,   36,   27,
    QMetaType::Void, 0x80000000 | 35, QMetaType::Int,   36,   27,
    QMetaType::Void, 0x80000000 | 35, QMetaType::Int,   36,   27,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QObjectStar, QMetaType::QString, QMetaType::QString,   41,   42,   43,
    QMetaType::Void, QMetaType::QObjectStar, QMetaType::QString,   41,   42,
    QMetaType::Void, QMetaType::QUrl,   45,
    QMetaType::Void, QMetaType::QString,   47,
    QMetaType::QString, QMetaType::QString, QMetaType::QUrl,   49,   45,
    QMetaType::QString, QMetaType::QString,   49,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QString, QMetaType::QUrl,   45,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   21,
    0x80000000 | 10, QMetaType::QString, 0x80000000 | 10,    9,   58,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 10,    9,   11,
    0x80000000 | 10, QMetaType::QString, 0x80000000 | 10,    9,   58,
    0x80000000 | 10, QMetaType::QString,    9,
    QMetaType::Bool, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      65, 0x80000000 | 35, 0x00015409, uint(-1), 0,
      66, QMetaType::QString, 0x00015401, uint(-1), 0,
      67, QMetaType::QString, 0x00015401, uint(-1), 0,
      68, 0x80000000 | 30, 0x00015409, uint(-1), 0,
      69, 0x80000000 | 30, 0x00015009, uint(0), 0,
      21, QMetaType::QString, 0x00015001, uint(1), 0,
      23, QMetaType::Int, 0x00015001, uint(1), 0,
      70, QMetaType::Int, 0x00015001, uint(2), 0,
      71, QMetaType::Bool, 0x00015001, uint(2), 0,
      72, QMetaType::Int, 0x00015001, uint(3), 0,
      73, QMetaType::QStringList, 0x00015001, uint(4), 0,
      74, QMetaType::QString, 0x00015401, uint(-1), 0,
      75, QMetaType::QString, 0x00015001, uint(5), 0,
      76, QMetaType::QString, 0x00015001, uint(5), 0,
      77, QMetaType::QStringList, 0x00015001, uint(5), 0,
      78, QMetaType::Int, 0x00015001, uint(10), 0,
      79, QMetaType::Bool, 0x00015001, uint(10), 0,
      80, QMetaType::Bool, 0x00015401, uint(-1), 0,
      81, QMetaType::Bool, 0x00015401, uint(-1), 0,
      82, QMetaType::QString, 0x00015401, uint(-1), 0,
      83, QMetaType::QString, 0x00015401, uint(-1), 0,
      84, QMetaType::Bool, 0x00015001, uint(9), 0,
      85, QMetaType::QString, 0x00015001, uint(9), 0,
      86, QMetaType::Bool, 0x00015001, uint(11), 0,
      87, QMetaType::QString, 0x00015001, uint(11), 0,
      88, QMetaType::QString, 0x00015001, uint(11), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject AppController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN13AppControllerE.offsetsAndSizes,
    qt_meta_data_ZN13AppControllerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN13AppControllerE_t,
        // property 'gallery'
        QtPrivate::TypeAndForceComplete<GalleryModel*, std::true_type>,
        // property 'version'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'releaseNotesVersion'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'releaseNotesSections'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'games'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'category'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'gameId'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'currentGameId'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'currentGameAvailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'overlayGameId'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'watchedFolders'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'capturesRoot'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'screenshotsRoot'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'clipsRoot'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'managedRoots'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'lastScanAdded'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'lastScanAvailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'startMinimized'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'portableMode'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'dataRoot'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'logsRoot'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'replayBufferActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'replayBufferGame'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'hdrDisplayActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'hdrStatusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'hdrDetailText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AppController, std::true_type>,
        // method 'gamesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'filterChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'currentGameChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlayGameChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'foldersChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'captureLocationsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'configChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariant &, std::false_type>,
        // method 'configGroupReset'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'replaySettingsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'replayBufferStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastScanChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'hdrStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'hdrDisplayConfigurationChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshHdrStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setCategory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setGame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setGameCategory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'rescan'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleFavorite'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'deleteCapture'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'deleteCaptures'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>,
        // method 'openCapture'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'showInFolder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'deleteCaptureFrom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<GalleryModel *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'openCaptureFrom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<GalleryModel *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'showInFolderFrom'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<GalleryModel *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'syncOverlayToForegroundGame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveVideoFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QObject *, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'saveVideoFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QObject *, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'addWatchedFolder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'removeWatchedFolder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setCaptureRoot'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'resetCaptureRoot'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'openDataFolder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'openLogsFolder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'beginPortableImport'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'quitApplication'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'copyDiagnosticSummary'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetCategory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'config'
        QtPrivate::TypeAndForceComplete<QVariant, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariant &, std::false_type>,
        // method 'setConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariant &, std::false_type>,
        // method 'configDefault'
        QtPrivate::TypeAndForceComplete<QVariant, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariant &, std::false_type>,
        // method 'configDefault'
        QtPrivate::TypeAndForceComplete<QVariant, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'configIsDefault'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resetConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resetConfigGroup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resetAllConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void AppController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AppController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->gamesChanged(); break;
        case 1: _t->filterChanged(); break;
        case 2: _t->currentGameChanged(); break;
        case 3: _t->overlayGameChanged(); break;
        case 4: _t->foldersChanged(); break;
        case 5: _t->captureLocationsChanged(); break;
        case 6: _t->configChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariant>>(_a[2]))); break;
        case 7: _t->configGroupReset((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->replaySettingsChanged(); break;
        case 9: _t->replayBufferStateChanged(); break;
        case 10: _t->lastScanChanged(); break;
        case 11: _t->hdrStatusChanged(); break;
        case 12: _t->hdrDisplayConfigurationChanged(); break;
        case 13: _t->refreshHdrStatus(); break;
        case 14: _t->setCategory((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->setGame((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 16: _t->setGameCategory((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 17: _t->rescan(); break;
        case 18: _t->toggleFavorite((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 19: _t->deleteCapture((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 20: _t->deleteCaptures((*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[1]))); break;
        case 21: _t->openCapture((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 22: _t->showInFolder((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 23: _t->deleteCaptureFrom((*reinterpret_cast< std::add_pointer_t<GalleryModel*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 24: _t->openCaptureFrom((*reinterpret_cast< std::add_pointer_t<GalleryModel*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 25: _t->showInFolderFrom((*reinterpret_cast< std::add_pointer_t<GalleryModel*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 26: _t->syncOverlayToForegroundGame(); break;
        case 27: _t->saveVideoFrame((*reinterpret_cast< std::add_pointer_t<QObject*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 28: _t->saveVideoFrame((*reinterpret_cast< std::add_pointer_t<QObject*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 29: _t->addWatchedFolder((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1]))); break;
        case 30: _t->removeWatchedFolder((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 31: { QString _r = _t->setCaptureRoot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 32: { QString _r = _t->resetCaptureRoot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 33: _t->openDataFolder(); break;
        case 34: _t->openLogsFolder(); break;
        case 35: { QString _r = _t->beginPortableImport((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 36: _t->quitApplication(); break;
        case 37: _t->copyDiagnosticSummary(); break;
        case 38: _t->resetCategory((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 39: { QVariant _r = _t->config((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariant>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QVariant*>(_a[0]) = std::move(_r); }  break;
        case 40: _t->setConfig((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariant>>(_a[2]))); break;
        case 41: { QVariant _r = _t->configDefault((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QVariant>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QVariant*>(_a[0]) = std::move(_r); }  break;
        case 42: { QVariant _r = _t->configDefault((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariant*>(_a[0]) = std::move(_r); }  break;
        case 43: { bool _r = _t->configIsDefault((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 44: _t->resetConfig((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 45: _t->resetConfigGroup((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 46: _t->resetAllConfig(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 23:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< GalleryModel* >(); break;
            }
            break;
        case 24:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< GalleryModel* >(); break;
            }
            break;
        case 25:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< GalleryModel* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::gamesChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::filterChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::currentGameChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::overlayGameChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::foldersChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::captureLocationsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)(const QString & , const QVariant & );
            if (_q_method_type _q_method = &AppController::configChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)(const QString & );
            if (_q_method_type _q_method = &AppController::configGroupReset; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::replaySettingsChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::replayBufferStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::lastScanChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::hdrStatusChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _q_method_type = void (AppController::*)();
            if (_q_method_type _q_method = &AppController::hdrDisplayConfigurationChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
    }
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< GalleryModel* >(); break;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< GalleryModel**>(_v) = _t->gallery(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->version(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->releaseNotesVersion(); break;
        case 3: *reinterpret_cast< QVariantList*>(_v) = _t->releaseNotesSections(); break;
        case 4: *reinterpret_cast< QVariantList*>(_v) = _t->games(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->category(); break;
        case 6: *reinterpret_cast< int*>(_v) = _t->gameId(); break;
        case 7: *reinterpret_cast< int*>(_v) = _t->currentGameId(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->currentGameAvailable(); break;
        case 9: *reinterpret_cast< int*>(_v) = _t->overlayGameId(); break;
        case 10: *reinterpret_cast< QStringList*>(_v) = _t->watchedFolders(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->capturesRoot(); break;
        case 12: *reinterpret_cast< QString*>(_v) = _t->screenshotsRoot(); break;
        case 13: *reinterpret_cast< QString*>(_v) = _t->clipsRoot(); break;
        case 14: *reinterpret_cast< QStringList*>(_v) = _t->managedRoots(); break;
        case 15: *reinterpret_cast< int*>(_v) = _t->lastScanAdded(); break;
        case 16: *reinterpret_cast< bool*>(_v) = _t->lastScanAvailable(); break;
        case 17: *reinterpret_cast< bool*>(_v) = _t->startMinimized(); break;
        case 18: *reinterpret_cast< bool*>(_v) = _t->portableMode(); break;
        case 19: *reinterpret_cast< QString*>(_v) = _t->dataRoot(); break;
        case 20: *reinterpret_cast< QString*>(_v) = _t->logsRoot(); break;
        case 21: *reinterpret_cast< bool*>(_v) = _t->replayBufferActive(); break;
        case 22: *reinterpret_cast< QString*>(_v) = _t->replayBufferGame(); break;
        case 23: *reinterpret_cast< bool*>(_v) = _t->hdrDisplayActive(); break;
        case 24: *reinterpret_cast< QString*>(_v) = _t->hdrStatusText(); break;
        case 25: *reinterpret_cast< QString*>(_v) = _t->hdrDetailText(); break;
        default: break;
        }
    }
}

const QMetaObject *AppController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AppController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN13AppControllerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AppController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 47)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 47;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 47)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 47;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 26;
    }
    return _id;
}

// SIGNAL 0
void AppController::gamesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AppController::filterChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AppController::currentGameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void AppController::overlayGameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void AppController::foldersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void AppController::captureLocationsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void AppController::configChanged(const QString & _t1, const QVariant & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void AppController::configGroupReset(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void AppController::replaySettingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void AppController::replayBufferStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void AppController::lastScanChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void AppController::hdrStatusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void AppController::hdrDisplayConfigurationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}
QT_WARNING_POP
