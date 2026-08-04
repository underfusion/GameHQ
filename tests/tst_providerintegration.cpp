#include "input/ProviderIntegration.h"
#include "input/CapabilityEventRouter.h"
#include "input/ControlId.h"
#include "input/PhysicalControllerRegistry.h"
#include "gameinput/StandardControlMap.h"

#include <QtTest>

using namespace ModernInput;

// t25 cross-provider integration: one PhysicalControllerRegistry and one
// CapabilityEventRouter shared by Sony Raw Input, GameInput, XInput, WinMM
// and the selective Raw HID fallback. These tests prove the core contract:
// one physical controller → one logical controller → exactly one edge per
// physical press, no matter how many providers observe it.
class ProviderIntegrationTest : public QObject
{
    Q_OBJECT

    static ProviderObservation gameInputObservation(
        const QString& deviceId = QStringLiteral("gi-device-1"),
        quint16 vendorId = 0x054C, quint16 productId = 0x0CE6)
    {
        ProviderObservation observation;
        observation.provider = ControllerProvider::GameInput;
        observation.providerDeviceId = deviceId;
        observation.appLocalDeviceId = deviceId;
        observation.displayName = QStringLiteral("Modern pad");
        observation.vendorId = vendorId;
        observation.productId = productId;
        observation.topologyRoot = QStringLiteral("%1:%2")
            .arg(vendorId, 4, 16, QLatin1Char('0'))
            .arg(productId, 4, 16, QLatin1Char('0'));
        observation.capabilities = ControllerCapability::StandardControls
            | ControllerCapability::SystemShare | ControllerCapability::Guide;
        return observation;
    }

    static ControllerCapabilities sonyCapabilities()
    {
        return ControllerCapability::StandardControls
            | ControllerCapability::SystemShare | ControllerCapability::Guide;
    }

private slots:
    void sonyAndGameInputMergeToOneLogicalController()
    {
        ProviderIntegration integration;
        const QString legacyId = integration.observeLegacy(
            ControllerProvider::SonyRaw, QStringLiteral("054c:0ce6"),
            QStringLiteral("054c:0ce6"), QStringLiteral("DualSense"),
            sonyCapabilities());
        QVERIFY(!legacyId.isEmpty());

        const QString modernId = integration.registry().observe(gameInputObservation());
        // The GameInput observation carries strong identity, so the merged
        // controller is renamed onto the deterministic strong hash — BOTH
        // attachments must resolve to that same logical controller.
        QCOMPARE(integration.registry().logicalIdFor(
                     ControllerProvider::SonyRaw, QStringLiteral("054c:0ce6")),
                 modernId);
        const auto* logical = integration.registry().controller(modernId);
        QVERIFY(logical);
        QCOMPARE(logical->providers.size(), 2);
        QVERIFY(integration.hasLegacyAttachment(modernId));
    }

    void mergedControllerFiresExactlyOneCaptureEdge()
    {
        ProviderIntegration integration;
        integration.observeLegacy(ControllerProvider::SonyRaw,
                                  QStringLiteral("054c:0ce6"), QStringLiteral("054c:0ce6"),
                                  QStringLiteral("DualSense"), sonyCapabilities());
        const QString logicalId = integration.registry().observe(gameInputObservation());

        // GameInput is the preferred Share provider on a merged controller:
        // its edge routes, the mirrored Sony Raw edge is refused.
        auto modern = integration.capabilityRouter().route(
            {logicalId, ControllerProvider::GameInput, ControllerCapability::SystemShare,
             ControlId::Capture, true, 100});
        QVERIFY(modern.accepted);
        const auto legacy = integration.routeLegacySystemEdge(
            ControllerProvider::SonyRaw, QStringLiteral("054c:0ce6"),
            ControlId::Capture, true, 105);
        QVERIFY(!legacy.accepted);

        auto modernRelease = integration.capabilityRouter().route(
            {logicalId, ControllerProvider::GameInput, ControllerCapability::SystemShare,
             ControlId::Capture, false, 180});
        QVERIFY(modernRelease.accepted);
    }

    void legacyOwnsShareOnceGameInputDetaches()
    {
        ProviderIntegration integration;
        integration.observeLegacy(ControllerProvider::SonyRaw,
                                  QStringLiteral("054c:0ce6"), QStringLiteral("054c:0ce6"),
                                  QStringLiteral("DualSense"), sonyCapabilities());
        const QString logicalId = integration.registry().observe(gameInputObservation());
        integration.registry().removeProvider(ControllerProvider::GameInput,
                                              QStringLiteral("gi-device-1"));
        Q_UNUSED(logicalId)

        const auto legacy = integration.routeLegacySystemEdge(
            ControllerProvider::SonyRaw, QStringLiteral("054c:0ce6"),
            ControlId::Capture, true, 100);
        QVERIFY(legacy.accepted);
    }

    void xinputGuideDedupsAgainstGameInput()
    {
        ProviderIntegration integration;
        integration.observeLegacy(ControllerProvider::XInput,
                                  QStringLiteral("045e:0b12"), QStringLiteral("045e:0b12"),
                                  QStringLiteral("Xbox pad"),
                                  ControllerCapability::StandardControls
                                      | ControllerCapability::Guide);
        const QString logicalId = integration.registry().observe(
            gameInputObservation(QStringLiteral("gi-xbox"), 0x045E, 0x0B12));
        QCOMPARE(integration.registry().logicalIdFor(
                     ControllerProvider::XInput, QStringLiteral("045e:0b12")),
                 logicalId);

        auto modern = integration.capabilityRouter().route(
            {logicalId, ControllerProvider::GameInput, ControllerCapability::Guide,
             ControlId::Guide, true, 50});
        QVERIFY(modern.accepted);
        const auto legacy = integration.routeLegacySystemEdge(
            ControllerProvider::XInput, QStringLiteral("045e:0b12"),
            ControlId::Guide, true, 55);
        QVERIFY(!legacy.accepted);
    }

    void identicalPadsStayDistinctAndUncorrelated()
    {
        ProviderIntegration integration;
        const QString first = integration.observeLegacy(
            ControllerProvider::SonyRaw, QStringLiteral("pad-a"),
            QStringLiteral("054c:0ce6"), QStringLiteral("DualSense"), sonyCapabilities());
        const QString second = integration.observeLegacy(
            ControllerProvider::SonyRaw, QStringLiteral("pad-b"),
            QStringLiteral("054c:0ce6"), QStringLiteral("DualSense"), sonyCapabilities());
        QVERIFY(first != second);

        // Same-model GameInput device: correlation is ambiguous, so it must
        // get its own identity with NO legacy attachment — the shadow gate's
        // trigger condition.
        const QString modernId = integration.registry().observe(gameInputObservation());
        QVERIFY(modernId != first);
        QVERIFY(modernId != second);
        QVERIFY(integration.legacyProviderConnected());
        QVERIFY(!integration.hasLegacyAttachment(modernId));
    }

    void unobservedLegacyEdgeFailsOpen()
    {
        ProviderIntegration integration;
        const auto result = integration.routeLegacySystemEdge(
            ControllerProvider::WinMM, QStringLiteral("never-observed"),
            ControlId::Guide, true, 10);
        QVERIFY(result.accepted);
    }

    void removeLegacyClearsAttachmentAndConnectionFlag()
    {
        ProviderIntegration integration;
        const QString logicalId = integration.observeLegacy(
            ControllerProvider::WinMM, QStringLiteral("054c:0ce6"),
            QStringLiteral("054c:0ce6"), QStringLiteral("DS4 (WinMM)"), sonyCapabilities());
        QVERIFY(integration.legacyProviderConnected());
        integration.removeLegacy(ControllerProvider::WinMM, QStringLiteral("054c:0ce6"));
        QVERIFY(!integration.legacyProviderConnected());
        QVERIFY(!integration.hasLegacyAttachment(logicalId));
    }

    void rawHidEdgeObservesCorrelatesAndRoutes()
    {
        ProviderIntegration integration;
        integration.observeLegacy(ControllerProvider::XInput,
                                  QStringLiteral("3537:1004"), QStringLiteral("3537:1004"),
                                  QStringLiteral("GameSir"),
                                  ControllerCapability::StandardControls
                                      | ControllerCapability::Guide);
        const QString control = ControlId::rawHidUsage(QStringLiteral("3537:1004"), 0x09, 0x15);
        const auto press = integration.routeRawHidEdge(QStringLiteral("3537:1004"),
                                                       control, true, 10);
        QVERIFY(press.accepted);
        // The Raw HID attachment correlates onto the SAME logical controller
        // the XInput attachment created — one physical pad, one identity.
        QCOMPARE(integration.registry().logicalIdFor(ControllerProvider::RawHid,
                                                     QStringLiteral("3537:1004")),
                 integration.registry().logicalIdFor(ControllerProvider::XInput,
                                                     QStringLiteral("3537:1004")));
        const auto duplicate = integration.routeRawHidEdge(QStringLiteral("3537:1004"),
                                                           control, true, 12);
        QVERIFY(!duplicate.accepted);
        const auto release = integration.routeRawHidEdge(QStringLiteral("3537:1004"),
                                                         control, false, 40);
        QVERIFY(release.accepted);
    }

    void weakIdentityUpgradesToDeterministicStrongId()
    {
        // Legacy observes first (weak identity), GameInput merges in with a
        // strong one: the logical ID must become the deterministic strong
        // hash so persisted per-controller state survives provider order.
        PhysicalControllerRegistry reference;
        const QString strongFirst = reference.observe(gameInputObservation());

        ProviderIntegration integration;
        integration.observeLegacy(ControllerProvider::SonyRaw,
                                  QStringLiteral("054c:0ce6"), QStringLiteral("054c:0ce6"),
                                  QStringLiteral("DualSense"), sonyCapabilities());
        const QString merged = integration.registry().observe(gameInputObservation());
        QCOMPARE(merged, strongFirst);
    }

    void standardControlMapCoversEveryPinnedHeaderFlag()
    {
        using namespace StandardControlMap;
        QCOMPARE(controlFor(FaceA), ControlId::FaceSouth);
        QCOMPARE(controlFor(FaceB), ControlId::FaceEast);
        QCOMPARE(controlFor(FaceX), ControlId::FaceWest);
        QCOMPARE(controlFor(FaceY), ControlId::FaceNorth);
        QCOMPARE(controlFor(FaceC), ControlId::FaceC);
        QCOMPARE(controlFor(FaceZ), ControlId::FaceZ);
        QCOMPARE(controlFor(Menu), ControlId::Menu);
        QCOMPARE(controlFor(View), ControlId::ViewBack);
        QCOMPARE(controlFor(LeftThumbstick), ControlId::ThumbLeft);
        QCOMPARE(controlFor(RightThumbstick), ControlId::ThumbRight);
        QCOMPARE(controlFor(LeftTriggerButton), ControlId::TriggerLeft);
        QCOMPARE(controlFor(RightTriggerButton), ControlId::TriggerRight);
        QCOMPARE(controlFor(PaddleLeft1), ControlId::PaddleLeft1);
        QCOMPARE(controlFor(PaddleRight2), ControlId::PaddleRight2);
        // Left-stick directions merge onto d-pad navigation, exactly like the
        // legacy backends' stick-nav path.
        QCOMPARE(controlFor(LeftThumbstickUp), ControlId::DpadUp);
        QCOMPARE(controlFor(LeftThumbstickRight), ControlId::DpadRight);
        // Right-stick directions have no canonical id — never invented.
        QVERIFY(controlFor(RightThumbstickUp).isEmpty());

        // Every mapped id is canonical and bindable.
        const auto controls = controlsFor(0xFFFFFFFFu);
        for (const QString& control : controls)
            QVERIFY2(ControlId::isCanonical(control), qPrintable(control));
    }

};

QTEST_GUILESS_MAIN(ProviderIntegrationTest)
#include "tst_providerintegration.moc"
