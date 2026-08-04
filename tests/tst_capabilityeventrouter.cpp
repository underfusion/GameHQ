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

    void silentPreferredProviderDoesNotBlockWorkingFallback()
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

        // A late preferred mirror transfers release ownership without a
        // second action, even outside the short timestamp dedup window.
        const auto modernMirror = router.route(
            {id, ControllerProvider::GameInput, ControllerCapability::StandardControls,
             ControlId::FaceSouth, true, 500});
        QVERIFY(modernMirror.duplicate);
        QVERIFY(!modernMirror.accepted);
        QVERIFY(router.route(
            {id, ControllerProvider::GameInput, ControllerCapability::StandardControls,
             ControlId::FaceSouth, false, 550}).accepted);
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
