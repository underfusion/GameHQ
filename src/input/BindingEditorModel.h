#pragma once

#include "input/BindingResolver.h"
#include "input/ControlId.h"

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
    Q_PROPERTY(QString lastFiredAction READ lastFiredAction NOTIFY lastFiredActionChanged)
public:
    BindingEditorModel(CaptureDatabase* database, BindingRuntime* runtime,
                       std::function<void()> reloadRuntime, QObject* parent = nullptr);

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
    QString lastFiredAction() const { return m_lastFiredAction; }

    Q_INVOKABLE void beginCapture(const QString& actionId, int slot);
    Q_INVOKABLE void cancelCapture();
    Q_INVOKABLE void clearBinding(const QString& actionId, int slot);
    Q_INVOKABLE void resetAction(const QString& actionId);
    Q_INVOKABLE void resetCurrentProfile();
    Q_INVOKABLE void resetAllBindings();
    Q_INVOKABLE void confirmConflict();
    Q_INVOKABLE void dismissConflict();

    bool captureInput(const QString& deviceGroup, const QString& triggerCode,
                      const QString& displayLabel);
    void setControllerProfile(const ControlId::DeviceProfile& profile);
    void setLastFiredAction(const QString& actionId);

signals:
    void deviceGroupChanged();
    void rowsChanged();
    void controllerSpecificChanged();
    void controllerProfileChanged();
    void captureChanged();
    void conflictChanged();
    void lastFiredActionChanged();

private:
    struct PendingChange {
        BindingResolver::Binding target;
        QVector<BindingResolver::Binding> conflicts;
    };

    QString selectedProfile() const;
    void rebuildRows();
    void applyChange(const PendingChange& change);
    void reloadAndRefresh();
    QString formatBinding(const BindingResolver::Binding& binding) const;
    static QString scopeLabel(ActionCatalog::Scope scope);
    static bool scopesConflict(ActionCatalog::Scope left, ActionCatalog::Scope right);

    CaptureDatabase* m_database = nullptr;
    BindingRuntime* m_runtime = nullptr;
    std::function<void()> m_reloadRuntime;
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
    PendingChange m_pending;
    QString m_lastFiredAction = QStringLiteral("No action fired yet");
};
