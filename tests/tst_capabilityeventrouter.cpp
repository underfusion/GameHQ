#include "input/CapabilityEventRouter.h"
#include "input/ControlId.h"

#include <QtTest>

using namespace ModernInput;

class CapabilityEventRouterTest : public QObject
{
    Q_OBJECT

    static QString mergedController(PhysicalControllerRegistry& registry)
    {
        ProviderObservation sony{ControllerProvider::SonyRaw, QStringLiteral("sony")};
        sony.containerId = QStringLiteral("container");
        sony.capabilities = ControllerCapability::StandardControls;
        ProviderObservation modern{ControllerProvider::GameInput, QStringLiteral("modern")};
        modern.containerId = sony.containerId;
        modern.capabilities = ControllerCapability::StandardControls
            | ControllerCapability::SystemShare | ControllerCapability::Guide
            | ControllerCapability::ExtraControls;
        const QString id = registry.observe(sony);
        const QString merged = registry.observe(modern);
        return merged == id ? id : QString();
    }

private slots:
    void sonyStandardAndGameInputShareComplementEachOther()
    {
        PhysicalControllerRegistry registry;
        const QString id = mergedController(registry);
        QVERIFY(!id.isEmpty());
        CapabilityEventRouter router(&registry);
        QVERIFY(router.route({id, ControllerProvider::SonyRaw,
                              ControllerCapability::StandardControls,
                              ControlId::FaceSouth, true, 100}).accepted);
        QVERIFY(!router.route({id, ControllerProvider::GameInput,
                               ControllerCapability::StandardControls,
                               ControlId::FaceSouth, true, 101}).accepted);
        QVERIFY(router.route({id, ControllerProvider::GameInput,
                              ControllerCapability::SystemShare,
                              ControlId::Capture, true, 102}).accepted);
    }

    void genericGameInputSuppressesLegacyMirror()
    {
        PhysicalControllerRegistry registry;
        ProviderObservation modern{ControllerProvider::GameInput, QStringLiteral("gi")};
        modern.containerId = QStringLiteral("same");
        modern.capabilities = ControllerCapability::StandardControls;
        ProviderObservation legacy{ControllerProvider::XInput, QStringLiteral("xi")};
        legacy.containerId = modern.containerId;
        legacy.capabilities = ControllerCapability::StandardControls;
        const QString id = registry.observe(modern);
        registry.observe(legacy);
        CapabilityEventRouter router(&registry);
        QVERIFY(router.route({id, ControllerProvider::GameInput,
                              ControllerCapability::StandardControls,
                              ControlId::FaceSouth, true, 200}).accepted);
        QVERIFY(!router.route({id, ControllerProvider::XInput,
                               ControllerCapability::StandardControls,
                               ControlId::FaceSouth, true, 201}).accepted);
    }

    void preferredMirrorJoinsCycleWithoutTakingOwnership()
    {
        PhysicalControllerRegistry registry;
        ProviderObservation modern{ControllerProvider::GameInput, QStringLiteral("gi")};
        modern.containerId = QStringLiteral("silent-same");
        modern.capabilities = ControllerCapability::StandardControls;
        modern.controls.insert(ControlId::FaceSouth);
        ProviderObservation legacy{ControllerProvider::XInput, QStringLiteral("xi")};
        legacy.containerId = modern.containerId;
        legacy.capabilities = ControllerCapability::StandardControls;
        legacy.controls.insert(ControlId::FaceSouth);
        const QString id = registry.observe(modern);
        registry.observe(legacy);

        CapabilityEventRouter router(&registry);
        const auto fallbackPress = router.route(
            {id, ControllerProvider::XInput, ControllerCapability::StandardControls,
             ControlId::FaceSouth, true, 200});
        QVERIFY(fallbackPress.accepted);

        // A late preferred mirror joins the open cycle as a participant but
        // never takes ownership and never emits a second action.
        const auto modernMirror = router.route(
            {id, ControllerProvider::GameInput, ControllerCapability::StandardControls,
             ControlId::FaceSouth, true, 500});
        QVERIFY(modernMirror.duplicate);
        QVERIFY(!modernMirror.accepted);
        // The participant's release may close the cycle...
        QVERIFY(router.route(
            {id, ControllerProvider::GameInput, ControllerCapability::StandardControls,
             ControlId::FaceSouth, false, 550}).accepted);
        // ...and the owner's late release is then explicitly ignored.
        const auto lateOwnerRelease = router.route(
            {id, ControllerProvider::XInput, ControllerCapability::StandardControls,
             ControlId::FaceSouth, false, 560});
        QVERIFY(!lateOwnerRelease.accepted);
        QVERIFY(lateOwnerRelease.safeReleases.isEmpty());
        // The next physical press starts a completely fresh cycle.
        QVERIFY(router.route(
            {id, ControllerProvider::XInput, ControllerCapability::StandardControls,
             ControlId::FaceSouth, true, 900}).accepted);
    }

    // The 0.7.4 Share/PS regression: Sony Raw owns the press, the preferred
    // GameInput mirror arrives while held, then Sony delivers the release.
    // Ownership must NOT transfer — the owner's release closes the cycle and
    // every following cycle must keep working.
    void ownerReleaseClosesCycleAfterPreferredMirrorHandoff()
    {
        PhysicalControllerRegistry registry;
        const QString id = mergedController(registry);
        QVERIFY(!id.isEmpty());
        CapabilityEventRouter router(&registry);
        for (const QString& control : {ControlId::Capture, ControlId::Guide}) {
            const ControllerCapability capability = control == ControlId::Capture
                ? ControllerCapability::SystemShare : ControllerCapability::Guide;
            quint64 t = 100;
            for (int cycle = 0; cycle < 5; ++cycle) {
                // A: owner press accepted.
                const auto press = router.route(
                    {id, ControllerProvider::SonyRaw, capability, control, true, t});
                QVERIFY2(press.accepted, qPrintable(
                    QStringLiteral("%1 cycle %2 press").arg(control).arg(cycle)));
                // B: preferred mirror press stays duplicate, no ownership move.
                const auto mirror = router.route(
                    {id, ControllerProvider::GameInput, capability, control, true, t + 8});
                QVERIFY(mirror.duplicate);
                QVERIFY(!mirror.accepted);
                // C: the owner's release closes the cycle exactly once.
                const auto release = router.route(
                    {id, ControllerProvider::SonyRaw, capability, control, false, t + 120});
                QVERIFY2(release.accepted, qPrintable(
                    QStringLiteral("%1 cycle %2 release").arg(control).arg(cycle)));
                // D: the mirror's late release is ignored — no spurious action.
                const auto lateMirror = router.route(
                    {id, ControllerProvider::GameInput, capability, control, false, t + 130});
                QVERIFY(!lateMirror.accepted);
                QVERIFY(lateMirror.safeReleases.isEmpty());
                t += 500;
            }
        }
    }

    void releaseWithoutOpenCycleIsIgnored()
    {
        PhysicalControllerRegistry registry;
        const QString id = mergedController(registry);
        QVERIFY(!id.isEmpty());
        CapabilityEventRouter router(&registry);
        // Cold stray release: no press cycle ever existed.
        const auto stray = router.route(
            {id, ControllerProvider::GameInput, ControllerCapability::SystemShare,
             ControlId::Capture, false, 50});
        QVERIFY(!stray.accepted);
        QVERIFY(stray.safeReleases.isEmpty());
        // The ignored release must not poison the next real cycle.
        QVERIFY(router.route(
            {id, ControllerProvider::SonyRaw, ControllerCapability::SystemShare,
             ControlId::Capture, true, 100}).accepted);
        QVERIFY(router.route(
            {id, ControllerProvider::SonyRaw, ControllerCapability::SystemShare,
             ControlId::Capture, false, 150}).accepted);
    }

    void nonParticipantReleaseNeverClosesALiveOwnersCycle()
    {
        PhysicalControllerRegistry registry;
        const QString id = mergedController(registry);
        QVERIFY(!id.isEmpty());
        CapabilityEventRouter router(&registry);
        QVERIFY(router.route(
            {id, ControllerProvider::SonyRaw, ControllerCapability::SystemShare,
             ControlId::Capture, true, 100}).accepted);
        // GameInput never pressed in this cycle; its release cannot close it.
        const auto foreign = router.route(
            {id, ControllerProvider::GameInput, ControllerCapability::SystemShare,
             ControlId::Capture, false, 120});
        QVERIFY(!foreign.accepted);
        QVERIFY(foreign.safeReleases.isEmpty());
        // The owner still closes its own cycle normally.
        QVERIFY(router.route(
            {id, ControllerProvider::SonyRaw, ControllerCapability::SystemShare,
             ControlId::Capture, false, 150}).accepted);
    }

    void ownershipIsIndependentForEachExtraControl()
    {
        PhysicalControllerRegistry registry;
        const QString gameExtra = QStringLiteral("controller.device_button:gi-layout:1");
        const QString rawExtra = ControlId::rawHidUsage(
            QStringLiteral("hid.endpoint.pad"), 0x09, 0x15);
        ProviderObservation modern{ControllerProvider::GameInput, QStringLiteral("gi")};
        modern.endpointId = QStringLiteral("hid.endpoint.pad");
        modern.capabilities = ControllerCapability::ExtraControls;
        modern.controls.insert(gameExtra);
        ProviderObservation raw{ControllerProvider::RawHid, QStringLiteral("raw")};
        raw.endpointId = modern.endpointId;
        raw.capabilities = ControllerCapability::ExtraControls;
        raw.controls.insert(rawExtra);
        const QString id = registry.observe(modern);
        QCOMPARE(registry.observe(raw), id);

        CapabilityEventRouter router(&registry);
        QVERIFY(router.route({id, ControllerProvider::RawHid,
                              ControllerCapability::ExtraControls,
                              rawExtra, true, 100}).accepted);
        QVERIFY(router.route({id, ControllerProvider::GameInput,
                              ControllerCapability::ExtraControls,
                              gameExtra, true, 101}).accepted);
        QVERIFY(router.route({id, ControllerProvider::RawHid,
                              ControllerCapability::ExtraControls,
                              rawExtra, false, 150}).accepted);
    }

    void fullResetClearsDedupAndFirstReconnectPressRoutes()
    {
        PhysicalControllerRegistry registry;
        ProviderObservation modern{ControllerProvider::GameInput, QStringLiteral("gi")};
        modern.appLocalDeviceId = QStringLiteral("strong");
        modern.capabilities = ControllerCapability::ExtraControls;
        const QString id = registry.observe(modern);
        CapabilityEventRouter router(&registry);
        const QString extra = ControlId::deviceButton(id, QStringLiteral("layout"), 39);
        QVERIFY(router.route({id, ControllerProvider::GameInput,
                              ControllerCapability::ExtraControls, extra, true, 300}).accepted);
        const auto duplicate = router.route({id, ControllerProvider::GameInput,
                                             ControllerCapability::ExtraControls,
                                             extra, true, 301});
        QVERIFY(duplicate.duplicate);
        const quint64 beforeReset = router.generation(id);
        QCOMPARE(router.resetLogicalController(id), QStringList{extra});
        QVERIFY(router.generation(id) > beforeReset);

        // No physical release arrived before disconnect. The first press of
        // the new device lifetime must not match the stale pressed edge.
        const auto reconnectPress = router.route(
            {id, ControllerProvider::GameInput,
             ControllerCapability::ExtraControls, extra, true, 900});
        QVERIFY(reconnectPress.accepted);
        QVERIFY(!reconnectPress.duplicate);
        QVERIFY(!reconnectPress.providerChanged);
    }

    void providerSwitchSynthesizesReleaseAndStartsANewGeneration()
    {
        PhysicalControllerRegistry registry;
        ProviderObservation modern{ControllerProvider::GameInput, QStringLiteral("gi")};
        modern.containerId = QStringLiteral("same");
        modern.capabilities = ControllerCapability::StandardControls;
        ProviderObservation legacy{ControllerProvider::XInput, QStringLiteral("xi")};
        legacy.containerId = modern.containerId;
        legacy.capabilities = ControllerCapability::StandardControls;
        const QString id = registry.observe(modern);
        registry.observe(legacy);
        CapabilityEventRouter router(&registry);
        const auto first = router.route({id, ControllerProvider::GameInput,
                                         ControllerCapability::StandardControls,
                                         ControlId::FaceSouth, true, 100});
        QVERIFY(first.accepted);
        QVERIFY(registry.removeProvider(ControllerProvider::GameInput, QStringLiteral("gi")));
        const auto switched = router.route({id, ControllerProvider::XInput,
                                            ControllerCapability::StandardControls,
                                            ControlId::FaceSouth, true, 110});
        QVERIFY(switched.accepted);
        QVERIFY(switched.providerChanged);
        QCOMPARE(switched.safeReleases, QStringList{ControlId::FaceSouth});
        QVERIFY(switched.generation > first.generation);
    }
};

QTEST_MAIN(CapabilityEventRouterTest)
#include "tst_capabilityeventrouter.moc"
