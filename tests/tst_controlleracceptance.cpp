#include "input/ActionCatalog.h"
#include "input/BindingPattern.h"
#include "input/BindingRuntime.h"
#include "input/ControlId.h"
#include "input/ProviderIntegration.h"
#include "storage/CaptureDatabase.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace ModernInput;

namespace {

class RuntimePipeline
{
public:
    explicit RuntimePipeline(CaptureDatabase* database)
        : database(database), runtime(database)
    {
        runtime.reload();
    }

    bool bind(const QString& profile, const QString& action, int slot,
              const QString& trigger)
    {
        const bool saved = database->upsertBindingOverride(
            {QStringLiteral("controller"), profile, action, slot, trigger,
             QStringLiteral("press"), 0, false, 1});
        runtime.reload();
        return saved;
    }

    bool deliver(const QString& logicalId, const QString& control, bool pressed,
                 const CapabilityRouteResult& route)
    {
        for (const QString& release : route.safeReleases)
            runtime.release(QStringLiteral("controller"), logicalId, release);
        if (!route.accepted)
            return false;
        if (pressed) {
            return runtime.press(QStringLiteral("controller"), logicalId, control,
                                 ActionCatalog::Scope::Global);
        }
        return runtime.release(QStringLiteral("controller"), logicalId, control);
    }

    CapabilityRouteResult direct(const QString& logicalId, ControllerProvider provider,
                                 ControllerCapability capability, const QString& control,
                                 bool pressed, quint64 timestamp)
    {
        const auto route = providers.capabilityRouter().route(
            {logicalId, provider, capability, control, pressed, timestamp});
        deliver(logicalId, control, pressed, route);
        return route;
    }

    CapabilityRouteResult legacy(ControllerProvider provider, const QString& providerId,
                                 const QString& control, bool pressed, quint64 timestamp)
    {
        const QString logicalId = providers.registry().logicalIdFor(provider, providerId);
        const auto route = providers.routeLegacySystemEdge(
            provider, providerId, control, pressed, timestamp);
        deliver(logicalId, control, pressed, route);
        return route;
    }

    CaptureDatabase* database = nullptr;
    BindingRuntime runtime;
    ProviderIntegration providers;
};

ProviderObservation modernObservation(const QString& providerId, const QString& endpoint)
{
    ProviderObservation observation;
    observation.provider = ControllerProvider::GameInput;
    observation.providerDeviceId = providerId;
    observation.appLocalDeviceId = providerId;
    observation.endpointId = endpoint;
    observation.displayName = QStringLiteral("Modern controller");
    observation.capabilities = ControllerCapability::StandardControls
        | ControllerCapability::SystemShare | ControllerCapability::Guide;
    observation.controls = {ControlId::FaceSouth, ControlId::Capture, ControlId::Guide};
    return observation;
}

} // namespace

class ControllerAcceptanceTest : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    CaptureDatabase* m_database = nullptr;

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_database = new CaptureDatabase(
            m_dir.filePath(QStringLiteral("gamehq.db")), this);
        QVERIFY(m_database->open());
    }

    void init()
    {
        QVERIFY(m_database->clearAllBindingOverrides());
    }

    void sonyAndGameInputSystemEdgesReachRuntimeExactlyOnce()
    {
        RuntimePipeline pipeline(m_database);
        const QString endpoint = QStringLiteral("hid.endpoint.dualsense-acceptance");
        pipeline.providers.observeLegacy(
            ControllerProvider::SonyRaw, QStringLiteral("sony-a"),
            QStringLiteral("054c:0ce6"), QStringLiteral("DualSense"),
            ControllerCapability::StandardControls | ControllerCapability::SystemShare
                | ControllerCapability::Guide,
            nullptr, endpoint);
        const QString logicalId = pipeline.providers.registry().observe(
            modernObservation(QStringLiteral("gameinput-a"), endpoint));
        QVERIFY(pipeline.bind(logicalId, QStringLiteral("global.screenshot"), 1,
                              ControlId::Capture));
        QVERIFY(pipeline.bind(logicalId, QStringLiteral("global.toggle_overlay"), 1,
                              ControlId::Guide));
        QSignalSpy actions(&pipeline.runtime, &BindingRuntime::actionTriggered);

        QVERIFY(pipeline.legacy(ControllerProvider::SonyRaw, QStringLiteral("sony-a"),
                                ControlId::Capture, true, 100).accepted);
        const auto shareMirror = pipeline.direct(
            logicalId, ControllerProvider::GameInput, ControllerCapability::SystemShare,
            ControlId::Capture, true, 250);
        QVERIFY(shareMirror.duplicate);
        QVERIFY(!shareMirror.accepted);
        QVERIFY(pipeline.direct(logicalId, ControllerProvider::GameInput,
                                ControllerCapability::SystemShare,
                                ControlId::Capture, false, 300).accepted);

        QVERIFY(pipeline.direct(logicalId, ControllerProvider::GameInput,
                                ControllerCapability::Guide,
                                ControlId::Guide, true, 400).accepted);
        QVERIFY(!pipeline.legacy(ControllerProvider::SonyRaw, QStringLiteral("sony-a"),
                                 ControlId::Guide, true, 450).accepted);
        QVERIFY(pipeline.direct(logicalId, ControllerProvider::GameInput,
                                ControllerCapability::Guide,
                                ControlId::Guide, false, 500).accepted);

        QCOMPARE(actions.size(), 2);
        QCOMPARE(actions.at(0).at(0).toString(), QStringLiteral("global.screenshot"));
        QCOMPARE(actions.at(1).at(0).toString(), QStringLiteral("global.toggle_overlay"));
    }

    void identicalXInputProfilesRemainIndependentAcrossReconnect()
    {
        RuntimePipeline pipeline(m_database);
        const auto capabilities = ControllerCapability::StandardControls
            | ControllerCapability::Guide;
        const QString first = pipeline.providers.observeLegacy(
            ControllerProvider::XInput, QStringLiteral("xinput.slot0"),
            QStringLiteral("045e:0b12"), QStringLiteral("Xbox A"), capabilities,
            nullptr, QStringLiteral("hid.endpoint.xbox-a"));
        const QString second = pipeline.providers.observeLegacy(
            ControllerProvider::XInput, QStringLiteral("xinput.slot1"),
            QStringLiteral("045e:0b12"), QStringLiteral("Xbox B"), capabilities,
            nullptr, QStringLiteral("hid.endpoint.xbox-b"));
        QVERIFY(first != second);
        QVERIFY(pipeline.bind(first, QStringLiteral("global.screenshot"), 2,
                              ControlId::FaceSouth));
        QVERIFY(pipeline.bind(second, QStringLiteral("global.save_replay"), 2,
                              ControlId::FaceSouth));
        QSignalSpy actions(&pipeline.runtime, &BindingRuntime::actionTriggered);

        QVERIFY(pipeline.runtime.press(QStringLiteral("controller"), first,
                                       ControlId::FaceSouth, ActionCatalog::Scope::Global));
        pipeline.runtime.release(QStringLiteral("controller"), first,
                                 ControlId::FaceSouth);
        QVERIFY(pipeline.runtime.press(QStringLiteral("controller"), second,
                                       ControlId::FaceSouth, ActionCatalog::Scope::Global));
        pipeline.runtime.release(QStringLiteral("controller"), second,
                                 ControlId::FaceSouth);
        QCOMPARE(actions.size(), 2);
        QCOMPARE(actions.at(0).at(0).toString(), QStringLiteral("global.screenshot"));
        QCOMPARE(actions.at(1).at(0).toString(), QStringLiteral("global.save_replay"));

        pipeline.providers.removeLegacy(ControllerProvider::XInput,
                                        QStringLiteral("xinput.slot0"));
        const QString reconnected = pipeline.providers.observeLegacy(
            ControllerProvider::XInput, QStringLiteral("xinput.slot0"),
            QStringLiteral("045e:0b12"), QStringLiteral("Xbox A"), capabilities,
            nullptr, QStringLiteral("hid.endpoint.xbox-a"));
        QCOMPARE(reconnected, first);
        QVERIFY(pipeline.runtime.press(QStringLiteral("controller"), reconnected,
                                       ControlId::FaceSouth, ActionCatalog::Scope::Global));
        QCOMPARE(actions.size(), 3);
        QCOMPARE(actions.last().at(0).toString(), QStringLiteral("global.screenshot"));
    }

    void rawHidDeviceBindingAndChordSurviveRuntimeRestart()
    {
        RuntimePipeline pipeline(m_database);
        const QString identity = QStringLiteral("hid.endpoint.raw-acceptance");
        const QString single = ControlId::rawHidUsage(identity, 0x09, 0x15);
        const QString chordFirst = ControlId::rawHidUsage(identity, 0x09, 0x16);
        const QString chordSecond = ControlId::rawHidUsage(identity, 0x09, 0x17);
        ProviderObservation raw;
        raw.provider = ControllerProvider::RawHid;
        raw.providerDeviceId = identity;
        raw.endpointId = identity;
        raw.capabilities = ControllerCapability::ExtraControls;
        raw.controls = {single, chordFirst, chordSecond};
        const QString logicalId = pipeline.providers.registry().observe(raw);
        QVERIFY(pipeline.bind(logicalId, QStringLiteral("global.screenshot"), 2, single));
        QVERIFY(pipeline.bind(logicalId, QStringLiteral("global.save_replay"), 2,
                              TriggerSpec::orderedChord(chordFirst, chordSecond).serialize()));
        QSignalSpy actions(&pipeline.runtime, &BindingRuntime::actionTriggered);

        pipeline.direct(logicalId, ControllerProvider::RawHid,
                        ControllerCapability::ExtraControls, single, true, 10);
        pipeline.direct(logicalId, ControllerProvider::RawHid,
                        ControllerCapability::ExtraControls, single, false, 20);
        pipeline.direct(logicalId, ControllerProvider::RawHid,
                        ControllerCapability::ExtraControls, chordFirst, true, 30);
        pipeline.direct(logicalId, ControllerProvider::RawHid,
                        ControllerCapability::ExtraControls, chordSecond, true, 40);
        pipeline.direct(logicalId, ControllerProvider::RawHid,
                        ControllerCapability::ExtraControls, chordSecond, false, 50);
        pipeline.direct(logicalId, ControllerProvider::RawHid,
                        ControllerCapability::ExtraControls, chordFirst, false, 60);
        QCOMPARE(actions.size(), 2);
        QCOMPARE(actions.at(0).at(0).toString(), QStringLiteral("global.screenshot"));
        QCOMPARE(actions.at(1).at(0).toString(), QStringLiteral("global.save_replay"));

        BindingRuntime restarted(m_database);
        restarted.reload();
        QSignalSpy restartedActions(&restarted, &BindingRuntime::actionTriggered);
        QVERIFY(restarted.press(QStringLiteral("controller"), logicalId, single,
                                ActionCatalog::Scope::Global));
        QCOMPARE(restartedActions.size(), 1);
        QCOMPARE(restartedActions.first().at(0).toString(),
                 QStringLiteral("global.screenshot"));
    }

    void heldDisconnectAndWeakToStrongRekeyReleaseCleanly()
    {
        RuntimePipeline pipeline(m_database);
        const QString providerId = QStringLiteral("sony-weak");
        const auto capabilities = ControllerCapability::SystemShare;
        const QString weak = pipeline.providers.observeLegacy(
            ControllerProvider::SonyRaw, providerId, QStringLiteral("054c:0ce6"),
            QStringLiteral("DualSense"), capabilities);
        QVERIFY(pipeline.bind(weak, QStringLiteral("global.screenshot"), 1,
                              ControlId::Capture));
        QSignalSpy actions(&pipeline.runtime, &BindingRuntime::actionTriggered);
        QVERIFY(pipeline.legacy(ControllerProvider::SonyRaw, providerId,
                                ControlId::Capture, true, 10).accepted);

        QStringList safeReleases;
        const QString strong = pipeline.providers.observeLegacy(
            ControllerProvider::SonyRaw, providerId, QStringLiteral("054c:0ce6"),
            QStringLiteral("DualSense"), capabilities, &safeReleases,
            QStringLiteral("hid.endpoint.sony-strong"));
        QVERIFY(strong != weak);
        QCOMPARE(safeReleases, QStringList{ControlId::Capture});
        for (const QString& control : safeReleases)
            pipeline.runtime.release(QStringLiteral("controller"), weak, control);
        pipeline.runtime.setProfileAlias(strong, weak);

        QVERIFY(pipeline.legacy(ControllerProvider::SonyRaw, providerId,
                                ControlId::Capture, true, 20).accepted);
        QVERIFY(pipeline.legacy(ControllerProvider::SonyRaw, providerId,
                                ControlId::Capture, false, 30).accepted);
        QCOMPARE(actions.size(), 2);

        QVERIFY(pipeline.legacy(ControllerProvider::SonyRaw, providerId,
                                ControlId::Capture, true, 40).accepted);
        const QStringList disconnected = pipeline.providers.removeLegacy(
            ControllerProvider::SonyRaw, providerId);
        QCOMPARE(disconnected, QStringList{ControlId::Capture});
        for (const QString& control : disconnected)
            pipeline.runtime.release(QStringLiteral("controller"), strong, control);
        int screenshots = 0;
        for (const auto& action : actions) {
            screenshots += action.at(0).toString()
                == QLatin1String("global.screenshot");
        }
        QCOMPARE(screenshots, 3);
    }
};

QTEST_GUILESS_MAIN(ControllerAcceptanceTest)
#include "tst_controlleracceptance.moc"
