#pragma once

#include "input/BindingRelation.h"
#include "input/BindingResolver.h"
#include "input/ControlId.h"
#include "storage/CaptureDatabase.h"

#include <QObject>
#include <QVariantList>
#include <functional>

class BindingRuntime;
class CaptureDatabase;

// QML-facing editor facade over the canonical binding resolver/runtime.
class BindingEditorModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString deviceGroup READ deviceGroup WRITE setDeviceGroup NOTIFY deviceGroupChanged)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(bool controllerSpecific READ controllerSpecific WRITE setControllerSpecific NOTIFY controllerSpecificChanged)
    Q_PROPERTY(bool controllerSpecificAvailable READ controllerSpecificAvailable NOTIFY controllerProfileChanged)
    Q_PROPERTY(QString controllerName READ controllerName NOTIFY controllerProfileChanged)
    Q_PROPERTY(bool captureActive READ captureActive NOTIFY captureChanged)
    Q_PROPERTY(QString capturePrompt READ capturePrompt NOTIFY captureChanged)
    Q_PROPERTY(bool conflictPending READ conflictPending NOTIFY conflictChanged)
    Q_PROPERTY(QString conflictMessage READ conflictMessage NOTIFY conflictChanged)
    // Binding notices live on their own channel. They must never be routed
    // through input.controllerWarning, which is reserved for HidHide/cloaked-pad
    // state — a key conflict overwriting a hidden-controller warning would hide
    // the one message the user cannot recover from on their own.
    Q_PROPERTY(QString relationNotice READ relationNotice NOTIFY relationNoticeChanged)
    Q_PROPERTY(QString relationKind READ relationKind NOTIFY relationNoticeChanged)
    Q_PROPERTY(QString validationError READ validationError NOTIFY relationNoticeChanged)
    Q_PROPERTY(QString lastFiredAction READ lastFiredAction NOTIFY lastFiredActionChanged)
    // True when the selected controller has a stable hardware identity and
    // pre-identity "any pad in this slot" override rows exist that the user
    // may want to promote to it. Promotion is always this explicit action —
    // slot rows are never silently rewritten or broadened.
    Q_PROPERTY(bool legacyCopyAvailable READ legacyCopyAvailable NOTIFY rowsChanged)
public:
    // Registers `chord` for actionId/slot with the OS, or releases the slot when
    // `chord` is empty. Returns false and fills `reason` when Windows refuses
    // the chord. Injected by InputEngine; left null in tests that do not care
    // about global hotkeys, and never called for non-keyboard device groups.
    using HotkeyApply = std::function<bool(const QString& actionId, int slot,
                                           const QString& chord, QString* reason)>;
    // Persists one override row. Swappable so a test can force the DB half of
    // the transaction to fail without corrupting a real database.
    using PersistRow = std::function<bool(const BindingOverrideRow& row)>;

    BindingEditorModel(CaptureDatabase* database, BindingRuntime* runtime,
                       std::function<void()> reloadRuntime, QObject* parent = nullptr);

    void setHotkeyApply(HotkeyApply apply);
    void setPersistRow(PersistRow persist);

    QString deviceGroup() const { return m_deviceGroup; }
    void setDeviceGroup(const QString& group);
    QVariantList rows() const { return m_rows; }
    bool controllerSpecific() const { return m_controllerSpecific; }
    void setControllerSpecific(bool specific);
    bool controllerSpecificAvailable() const { return !m_controllerFingerprint.isEmpty(); }
    QString controllerName() const { return m_controllerName; }
    bool captureActive() const { return m_captureActive; }
    QString capturePrompt() const { return m_capturePrompt; }
    bool conflictPending() const { return m_conflictPending; }
    QString conflictMessage() const { return m_conflictMessage; }
    QString relationNotice() const { return m_relationNotice; }
    QString relationKind() const { return m_relationKind; }
    QString validationError() const { return m_validationError; }
    QString lastFiredAction() const { return m_lastFiredAction; }

    Q_INVOKABLE void beginCapture(const QString& actionId, int slot);
    Q_INVOKABLE void cancelCapture();
    Q_INVOKABLE void clearBinding(const QString& actionId, int slot);
    Q_INVOKABLE void resetAction(const QString& actionId);
    Q_INVOKABLE void resetCurrentProfile();
    Q_INVOKABLE void resetAllBindings();
    Q_INVOKABLE void confirmConflict();
    Q_INVOKABLE void dismissConflict();
    Q_INVOKABLE void dismissRelationNotice();
    // Re-opens capture for the same action/slot so the user can pick another
    // trigger without hunting for the row again ("Choose another" in the dialog).
    Q_INVOKABLE void retryConflictCapture();
    bool legacyCopyAvailable() const;
    Q_INVOKABLE void copyLegacyOverridesToController();

    bool captureInput(const QString& deviceGroup, const QString& triggerCode,
                      const QString& displayLabel);
    // Fourth notice tier. Raised when a backend knows a physical button exists
    // but cannot deliver it as a logical control — the GameSir case, where the
    // pad's capture button never reaches us. No producer is wired yet; the
    // backend that can tell the difference arrives with the GameInput work.
    void reportUnsupportedInput(const QString& displayLabel);
    void setControllerProfile(const ControlId::DeviceProfile& profile);
    void setLastFiredAction(const QString& actionId);

signals:
    void deviceGroupChanged();
    void rowsChanged();
    void controllerSpecificChanged();
    void controllerProfileChanged();
    void captureChanged();
    void conflictChanged();
    void relationNoticeChanged();
    void lastFiredActionChanged();

private:
    struct PendingChange {
        BindingResolver::Binding target;
        QVector<BindingResolver::Binding> conflicts;
    };

    QString selectedProfile() const;
    void rebuildRows();
    bool applyChange(const PendingChange& change);
    // True when this binding owns a Win32 global hotkey, i.e. the only rows the
    // OS half of the transaction applies to.
    bool isGlobalHotkey(const BindingResolver::Binding& binding) const;
    bool persist(const BindingOverrideRow& row);
    void reloadAndRefresh();
    QString formatBinding(const BindingResolver::Binding& binding) const;
    void setRelationNotice(const QString& kindId, const QString& text);
    QString noticeTextFor(BindingRelation::Kind kind,
                          const BindingResolver::Binding& target,
                          const BindingResolver::Binding& partner,
                          const QString& displayLabel) const;
    static QString gestureLabel(const QString& activation);
    static QString scopeLabel(ActionCatalog::Scope scope);

    CaptureDatabase* m_database = nullptr;
    BindingRuntime* m_runtime = nullptr;
    std::function<void()> m_reloadRuntime;
    HotkeyApply m_hotkeyApply;
    PersistRow m_persistRow;
    QString m_deviceGroup = QStringLiteral("controller");
    QVariantList m_rows;
    bool m_controllerSpecific = false;
    QString m_controllerFingerprint;
    QString m_controllerName;
    ControlId::ControllerFamily m_controllerFamily = ControlId::ControllerFamily::Generic;
    bool m_captureActive = false;
    QString m_captureActionId;
    int m_captureSlot = 1;
    QString m_capturePrompt;
    bool m_conflictPending = false;
    QString m_conflictMessage;
    QString m_relationNotice;
    QString m_relationKind = QStringLiteral("none");
    QString m_validationError;
    PendingChange m_pending;
    QString m_lastFiredAction = QStringLiteral("No action fired yet");
};
