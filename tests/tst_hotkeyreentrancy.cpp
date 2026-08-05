#include <QtTest>

#include "input/HotkeyManager.h"

#include <windows.h>

// 0.7.3 hardening regression for HotkeyManager::dispatch: it used to keep a
// reference into m_bindings across its synchronous signal emits. A slot that
// clears or rebinds the very binding being dispatched invalidates that
// reference (QHash erase/rehash), so the final hotkeyTriggered emit could read
// freed memory. dispatch now copies the action id before the first emit.
//
// WM_HOTKEY is synthesized straight into nativeEventFilter — no keyboard
// injection, so the test is deterministic. The chords are real RegisterHotKey
// registrations, chosen deliberately exotic (Ctrl+Alt+Shift+F22..F24); if
// another process happens to own one, the test skips rather than fails.
class HotkeyReentrancyTest : public QObject
{
    Q_OBJECT

    // Deliver WM_HOTKEY for whatever id the manager registered. Ids are
    // assigned internally, so probe the small id space instead of peeking
    // at private state; the filter returns false for unknown ids.
    static bool fire(HotkeyManager& hotkeys)
    {
        for (int id = 1; id <= 32; ++id) {
            MSG msg = {};
            msg.message = WM_HOTKEY;
            msg.wParam = static_cast<WPARAM>(id);
            qintptr result = 0;
            if (hotkeys.nativeEventFilter(QByteArrayLiteral("windows_generic_MSG"),
                                          &msg, &result))
                return true;
        }
        return false;
    }

private slots:
    void slotClearingItsOwnBindingStillDeliversTheAction()
    {
        HotkeyManager hotkeys;
        const QString action = QStringLiteral("global.toggle_overlay");
        if (!hotkeys.applyBinding(action,
                                  MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT,
                                  VK_F24))
            QSKIP("Ctrl+Alt+Shift+F24 is registered by another process");

        int overlayEdges = 0;
        QStringList triggered;
        connect(&hotkeys, &HotkeyManager::overlayTogglePressed, &hotkeys,
                [&hotkeys, &action, &overlayEdges] {
                    ++overlayEdges;
                    hotkeys.clearBinding(action);   // erases the dispatched binding
                });
        connect(&hotkeys, &HotkeyManager::hotkeyTriggered, &hotkeys,
                [&triggered](const QString& id) { triggered.append(id); });

        QVERIFY(fire(hotkeys));
        QCOMPARE(overlayEdges, 1);
        QCOMPARE(triggered, QStringList{ action });

        // The binding really is gone: nothing left to fire.
        QVERIFY(!fire(hotkeys));
    }

    void slotRebindingItsOwnBindingStillDeliversTheAction()
    {
        HotkeyManager hotkeys;
        const QString action = QStringLiteral("global.toggle_overlay");
        if (!hotkeys.applyBinding(action,
                                  MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT,
                                  VK_F23))
            QSKIP("Ctrl+Alt+Shift+F23 is registered by another process");

        QStringList triggered;
        connect(&hotkeys, &HotkeyManager::overlayTogglePressed, &hotkeys,
                [&hotkeys, &action] {
                    // Replace the dispatched binding, then grow the table so a
                    // rehash of m_bindings can relocate its nodes mid-dispatch.
                    hotkeys.applyBinding(action,
                                         MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT,
                                         VK_F22);
                    for (int i = 0; i < 8; ++i)
                        hotkeys.applyBinding(QStringLiteral("test.aux%1").arg(i),
                                             MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT,
                                             VK_F13 + i);
                });
        connect(&hotkeys, &HotkeyManager::hotkeyTriggered, &hotkeys,
                [&triggered](const QString& id) { triggered.append(id); });

        QVERIFY(fire(hotkeys));
        QCOMPARE(triggered, QStringList{ action });
    }
};

QTEST_GUILESS_MAIN(HotkeyReentrancyTest)
#include "tst_hotkeyreentrancy.moc"
