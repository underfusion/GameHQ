#include "input/ActionCatalog.h"
#include "input/BindingEditorModel.h"
#include "input/BindingRuntime.h"
#include "input/ControlId.h"
#include "storage/CaptureDatabase.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QVariantMap>

namespace
{
QVariantMap rowFor(const BindingEditorModel &editor, const QString &actionId)
{
    for (const QVariant &entry : editor.rows()) {
        const QVariantMap row = entry.toMap();
        if (row.value(QStringLiteral("actionId")).toString() == actionId)
            return row;
    }
    return {};
}

bool hasBinding(const BindingRuntime &runtime, const QString &group, const QString &profile,
                const QString &actionId, int slot, const QString &trigger)
{
    for (const auto &binding : runtime.effectiveBindings(group, profile)) {
        if (binding.actionId == actionId && binding.slot == slot
            && binding.triggerCode == trigger)
            return true;
    }
    return false;
}
}

class BindingEditorTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    CaptureDatabase *m_database = nullptr;
    BindingRuntime *m_runtime = nullptr;
    BindingEditorModel *m_editor = nullptr;

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_database = new CaptureDatabase(m_dir.filePath(QStringLiteral("gamehq.db")), this);
        QVERIFY(m_database->open());
        m_runtime = new BindingRuntime(m_database, this);
        m_runtime->reload();
        m_editor = new BindingEditorModel(m_database, m_runtime, [this] {
            m_runtime->reload();
        }, this);
    }

    void init()
    {
        QVERIFY(m_database->clearAllBindingOverrides());
        m_runtime->reload();
        m_runtime->cancelAll();
        m_editor->cancelCapture();
        m_editor->dismissConflict();
        m_editor->setDeviceGroup(QStringLiteral("controller"));
        m_editor->setControllerProfile({});
        m_editor->setControllerSpecific(false);
    }

    void editsBothSlotsPersistsAndResets()
    {
        m_editor->setDeviceGroup(QStringLiteral("keyboard"));
        m_editor->beginCapture(QStringLiteral("global.screenshot"), 2);
        QVERIFY(m_editor->captureInput(QStringLiteral("keyboard"),
                                       QStringLiteral("Ctrl+Alt+P"),
                                       QStringLiteral("Ctrl+Alt+P")));

        const QVariantMap edited = rowFor(*m_editor, QStringLiteral("global.screenshot"));
        QCOMPARE(edited.value(QStringLiteral("primary")).toString(),
                 QStringLiteral("Ctrl+Shift+S"));
        QCOMPARE(edited.value(QStringLiteral("secondary")).toString(),
                 QStringLiteral("Ctrl+Alt+P"));

        BindingRuntime reloaded(m_database);
        reloaded.reload();
        QVERIFY(hasBinding(reloaded, QStringLiteral("keyboard"), {},
                           QStringLiteral("global.screenshot"), 2,
                           QStringLiteral("Ctrl+Alt+P")));

        m_editor->clearBinding(QStringLiteral("global.screenshot"), 1);
        QVERIFY(!hasBinding(*m_runtime, QStringLiteral("keyboard"), {},
                            QStringLiteral("global.screenshot"), 1,
                            QStringLiteral("Ctrl+Shift+S")));
        m_editor->resetAction(QStringLiteral("global.screenshot"));
        QVERIFY(hasBinding(*m_runtime, QStringLiteral("keyboard"), {},
                           QStringLiteral("global.screenshot"), 1,
                           QStringLiteral("Ctrl+Shift+S")));
        QCOMPARE(rowFor(*m_editor, QStringLiteral("global.screenshot"))
                     .value(QStringLiteral("secondary")).toString(),
                 QStringLiteral("Unassigned"));
    }

    void conflictCanBeCanceledOrReplaced()
    {
        m_editor->setDeviceGroup(QStringLiteral("keyboard"));
        m_editor->beginCapture(QStringLiteral("desktop.navigate_down"), 2);
        QVERIFY(m_editor->captureInput(QStringLiteral("keyboard"), QStringLiteral("Up"),
                                       QStringLiteral("Up")));
        QVERIFY(m_editor->conflictPending());
        m_editor->dismissConflict();
        QVERIFY(hasBinding(*m_runtime, QStringLiteral("keyboard"), {},
                           QStringLiteral("desktop.navigate_up"), 1, QStringLiteral("Up")));
        QVERIFY(!hasBinding(*m_runtime, QStringLiteral("keyboard"), {},
                            QStringLiteral("desktop.navigate_down"), 2, QStringLiteral("Up")));

        m_editor->beginCapture(QStringLiteral("desktop.navigate_down"), 2);
        QVERIFY(m_editor->captureInput(QStringLiteral("keyboard"), QStringLiteral("Up"),
                                       QStringLiteral("Up")));
        QVERIFY(m_editor->conflictPending());
        m_editor->confirmConflict();
        QVERIFY(!hasBinding(*m_runtime, QStringLiteral("keyboard"), {},
                            QStringLiteral("desktop.navigate_up"), 1, QStringLiteral("Up")));
        QVERIFY(hasBinding(*m_runtime, QStringLiteral("keyboard"), {},
                           QStringLiteral("desktop.navigate_down"), 2, QStringLiteral("Up")));
    }

    void deviceSpecificOverrideDoesNotLeakAndProfileResetIsScoped()
    {
        const ControlId::DeviceProfile dualSense{
            QStringLiteral("Sony Raw Input"), QStringLiteral("054C:0CE6"),
            ControlId::ControllerFamily::PlayStation, QStringLiteral("DualSense")
        };
        m_editor->setControllerProfile(dualSense);
        m_editor->setControllerSpecific(true);
        QVERIFY(m_editor->controllerSpecific());

        const QString custom = ControlId::genericButton(17);
        m_editor->beginCapture(QStringLiteral("desktop.favorite"), 2);
        QVERIFY(m_editor->captureInput(QStringLiteral("controller"), custom,
                                       QStringLiteral("Button 17")));
        QVERIFY(hasBinding(*m_runtime, QStringLiteral("controller"),
                           QStringLiteral("054C:0CE6"),
                           QStringLiteral("desktop.favorite"), 2, custom));
        QVERIFY(!hasBinding(*m_runtime, QStringLiteral("controller"), {},
                            QStringLiteral("desktop.favorite"), 2, custom));

        m_editor->resetCurrentProfile();
        QVERIFY(!hasBinding(*m_runtime, QStringLiteral("controller"),
                            QStringLiteral("054C:0CE6"),
                            QStringLiteral("desktop.favorite"), 2, custom));
    }

    void mouseInputsPersistWithFriendlyLabels()
    {
        m_editor->setDeviceGroup(QStringLiteral("mouse"));
        m_editor->beginCapture(QStringLiteral("global.screenshot"), 1);
        QVERIFY(m_editor->captureInput(QStringLiteral("mouse"),
                                       QStringLiteral("mouse.middle"),
                                       QStringLiteral("Middle Mouse")));
        QCOMPARE(rowFor(*m_editor, QStringLiteral("global.screenshot"))
                     .value(QStringLiteral("primary")).toString(),
                 QStringLiteral("Middle Mouse"));

        BindingRuntime reloaded(m_database);
        reloaded.reload();
        QVERIFY(hasBinding(reloaded, QStringLiteral("mouse"), {},
                           QStringLiteral("global.screenshot"), 1,
                           QStringLiteral("mouse.middle")));
        m_editor->resetAllBindings();
        QVERIFY(m_database->listBindingOverrides().isEmpty());
    }

    void fixedBackActionsCannotBeCapturedOrCleared()
    {
        const QVariantMap overlayBack = rowFor(*m_editor, QStringLiteral("overlay.back"));
        const QVariantMap desktopBack = rowFor(*m_editor, QStringLiteral("desktop.back"));
        QCOMPARE(overlayBack.value(QStringLiteral("bindable")).toBool(), false);
        QCOMPARE(desktopBack.value(QStringLiteral("bindable")).toBool(), false);

        m_editor->beginCapture(QStringLiteral("desktop.back"), 1);
        QVERIFY(!m_editor->captureActive());
        m_editor->clearBinding(QStringLiteral("desktop.back"), 1);
        QVERIFY(m_database->listBindingOverrides().isEmpty());
        QVERIFY(hasBinding(*m_runtime, QStringLiteral("controller"), {},
                           QStringLiteral("desktop.back"), 1, ControlId::FaceEast));
    }

    void runtimeSeparatesTapHoldAndDoubleTap()
    {
        m_runtime->setDefaultHoldMs(250);
        m_runtime->reload();
        QSignalSpy spy(m_runtime, &BindingRuntime::actionTriggered);

        QVERIFY(m_runtime->press(QStringLiteral("controller"), {}, ControlId::Capture,
                                 ActionCatalog::Scope::Desktop,
                                 ActionCatalog::Scope::Overlay));
        QVERIFY(m_runtime->release(QStringLiteral("controller"), {}, ControlId::Capture));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 700);
        QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("global.screenshot"));

        QVERIFY(m_runtime->press(QStringLiteral("controller"), {}, ControlId::Capture,
                                 ActionCatalog::Scope::Desktop,
                                 ActionCatalog::Scope::Overlay));
        QTest::qWait(350);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("global.save_replay"));
        QVERIFY(m_runtime->release(QStringLiteral("controller"), {}, ControlId::Capture));
        QTest::qWait(350);
        QCOMPARE(spy.count(), 0);

        QVERIFY(m_runtime->press(QStringLiteral("controller"), {}, ControlId::Capture,
                                 ActionCatalog::Scope::Desktop,
                                 ActionCatalog::Scope::Overlay));
        QVERIFY(m_runtime->release(QStringLiteral("controller"), {}, ControlId::Capture));
        QTest::qWait(50);
        QVERIFY(m_runtime->press(QStringLiteral("controller"), {}, ControlId::Capture,
                                 ActionCatalog::Scope::Desktop,
                                 ActionCatalog::Scope::Overlay));
        QVERIFY(m_runtime->release(QStringLiteral("controller"), {}, ControlId::Capture));
        QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 300);
        QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("global.toggle_overlay"));
    }
};

QTEST_GUILESS_MAIN(BindingEditorTest)
#include "tst_bindingeditor.moc"
