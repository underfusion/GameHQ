#include "input/ProviderIntegration.h"
#include "input/CapabilityEventRouter.h"
#include "input/ControlId.h"
#include "input/PhysicalControllerRegistry.h"
#include "gameinput/GameInputLabelMap.h"
#include "gameinput/StandardControlMap.h"

#include "GameInput.h"

#include <QtTest>

using namespace ModernInput;

namespace GI = GameInput::v3;

#define GAMEHQ_ASSERT_GAMEINPUT_FLAG(localName, sdkName) \
    static_assert(StandardControlMap::localName == quint32(GI::sdkName))

GAMEHQ_ASSERT_GAMEINPUT_FLAG(Menu, GameInputGamepadMenu);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(View, GameInputGamepadView);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(FaceA, GameInputGamepadA);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(FaceB, GameInputGamepadB);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(FaceC, GameInputGamepadC);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(FaceX, GameInputGamepadX);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(FaceY, GameInputGamepadY);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(FaceZ, GameInputGamepadZ);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(DpadUp, GameInputGamepadDPadUp);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(DpadDown, GameInputGamepadDPadDown);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(DpadLeft, GameInputGamepadDPadLeft);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(DpadRight, GameInputGamepadDPadRight);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(LeftShoulder, GameInputGamepadLeftShoulder);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(RightShoulder, GameInputGamepadRightShoulder);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(LeftTriggerButton, GameInputGamepadLeftTriggerButton);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(RightTriggerButton, GameInputGamepadRightTriggerButton);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(LeftThumbstick, GameInputGamepadLeftThumbstick);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(RightThumbstick, GameInputGamepadRightThumbstick);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(LeftThumbstickUp, GameInputGamepadLeftThumbstickUp);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(LeftThumbstickDown, GameInputGamepadLeftThumbstickDown);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(LeftThumbstickLeft, GameInputGamepadLeftThumbstickLeft);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(LeftThumbstickRight, GameInputGamepadLeftThumbstickRight);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(RightThumbstickUp, GameInputGamepadRightThumbstickUp);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(RightThumbstickDown, GameInputGamepadRightThumbstickDown);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(RightThumbstickLeft, GameInputGamepadRightThumbstickLeft);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(RightThumbstickRight, GameInputGamepadRightThumbstickRight);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(PaddleLeft1, GameInputGamepadPaddleLeft1);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(PaddleLeft2, GameInputGamepadPaddleLeft2);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(PaddleRight1, GameInputGamepadPaddleRight1);
GAMEHQ_ASSERT_GAMEINPUT_FLAG(PaddleRight2, GameInputGamepadPaddleRight2);
static_assert(SystemControlMap::Guide == quint32(GI::GameInputSystemButtonGuide));
static_assert(SystemControlMap::Share == quint32(GI::GameInputSystemButtonShare));

#undef GAMEHQ_ASSERT_GAMEINPUT_FLAG

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
        quint16 vendorId = 0x054C, quint16 productId = 0x0CE6,
        const QString& deviceRoot = QStringLiteral("pnp.root.default"))
    {
        ProviderObservation observation;
        observation.provider = ControllerProvider::GameInput;
        observation.providerDeviceId = deviceId;
        observation.appLocalDeviceId = deviceId;
        observation.displayName = QStringLiteral("Modern pad");
        observation.vendorId = vendorId;
        observation.productId = productId;
        observation.topologyRoot = deviceRoot;
        observation.modelFingerprint = QStringLiteral("%1:%2")
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
            sonyCapabilities(), nullptr, {}, {}, QStringLiteral("pnp.root.default"));
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
                                  QStringLiteral("DualSense"), sonyCapabilities(), nullptr,
                                  {}, {}, QStringLiteral("pnp.root.default"));
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
                                  QStringLiteral("DualSense"), sonyCapabilities(), nullptr,
                                  {}, {}, QStringLiteral("pnp.root.default"));
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
                                      | ControllerCapability::Guide, nullptr, {}, {},
                                  QStringLiteral("pnp.root.xbox"));
        const QString logicalId = integration.registry().observe(
            gameInputObservation(QStringLiteral("gi-xbox"), 0x045E, 0x0B12,
                                 QStringLiteral("pnp.root.xbox")));
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

    void sameVidPidAcrossDifferentProvidersNeverMerges()
    {
        ProviderIntegration integration;
        const QString sony = integration.observeLegacy(
            ControllerProvider::SonyRaw, QStringLiteral("hid.endpoint.sony-a"),
            QStringLiteral("054c:0ce6"), QStringLiteral("DualSense A"),
            sonyCapabilities(), nullptr, QStringLiteral("hid.endpoint.sony-a"));
        const QString winmm = integration.observeLegacy(
            ControllerProvider::WinMM, QStringLiteral("winmm.slot1"),
            QStringLiteral("054c:0ce6"), QStringLiteral("DualSense B"),
            sonyCapabilities(), nullptr, QStringLiteral("winmm.slot1"));

        QVERIFY(sony != winmm);
        QCOMPARE(integration.registry().controllers().size(), 2);
    }

    void viewFallbackRequiresLegacyXInputWithoutTrueShareOrExplicitBinding()
    {
        ProviderIntegration integration;
        const QString slot = QStringLiteral("xinput.slot0");
        integration.observeLegacy(
            ControllerProvider::XInput, slot, QStringLiteral("045e:0b12"),
            QStringLiteral("Xbox pad"), ControllerCapability::StandardControls
                | ControllerCapability::Guide, nullptr, {}, {},
            QStringLiteral("pnp.root.view-test"));

        QVERIFY(integration.allowsLegacyViewFallback(
            ControllerProvider::XInput, slot, false));
        QVERIFY(!integration.allowsLegacyViewFallback(
            ControllerProvider::XInput, slot, true));
        QVERIFY(!integration.allowsLegacyViewFallback(
            ControllerProvider::SonyRaw, slot, false));

        auto modern = gameInputObservation(QStringLiteral("gi-view-test"),
                                           0x045E, 0x0B12,
                                           QStringLiteral("pnp.root.view-test"));
        modern.capabilities |= ControllerCapability::SystemShare;
        integration.registry().observe(modern);
        QVERIFY(!integration.allowsLegacyViewFallback(
            ControllerProvider::XInput, slot, false));
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

    void legacyProviderReplacementReleasesHeldAndResetsDedup()
    {
        ProviderIntegration integration;
        const QString device = QStringLiteral("xinput.slot0");
        integration.observeLegacy(ControllerProvider::XInput, device,
                                  QStringLiteral("045e:0b12"),
                                  QStringLiteral("Xbox pad"),
                                  ControllerCapability::StandardControls
                                      | ControllerCapability::Guide);
        QVERIFY(integration.routeLegacySystemEdge(ControllerProvider::XInput, device,
                                                  ControlId::Guide, true, 10).accepted);
        QCOMPARE(integration.removeLegacy(ControllerProvider::XInput, device),
                 QStringList{ControlId::Guide});

        integration.observeLegacy(ControllerProvider::XInput, device,
                                  QStringLiteral("045e:0b12"),
                                  QStringLiteral("Xbox pad"),
                                  ControllerCapability::StandardControls
                                      | ControllerCapability::Guide);
        const auto firstAfterReplacement = integration.routeLegacySystemEdge(
            ControllerProvider::XInput, device, ControlId::Guide, true, 1000);
        QVERIFY(firstAfterReplacement.accepted);
        QVERIFY(!firstAfterReplacement.duplicate);
    }

    void rawHidEdgeObservesCorrelatesAndRoutes()
    {
        ProviderIntegration integration;
        const QString endpoint = QStringLiteral("hid.endpoint.gamesir-a");
        integration.observeLegacy(ControllerProvider::XInput,
                                  endpoint, QStringLiteral("3537:1004"),
                                  QStringLiteral("GameSir"),
                                  ControllerCapability::StandardControls
                                      | ControllerCapability::Guide, nullptr, endpoint);
        const QString control = ControlId::rawHidUsage(endpoint, 0x09, 0x15);
        const auto press = integration.routeRawHidEdge(endpoint,
                                                       control, true, 10);
        QVERIFY(press.accepted);
        // The Raw HID attachment correlates onto the SAME logical controller
        // the XInput attachment created — one physical pad, one identity.
        QCOMPARE(integration.registry().logicalIdFor(ControllerProvider::RawHid,
                                                     endpoint),
                 integration.registry().logicalIdFor(ControllerProvider::XInput,
                                                     endpoint));
        const auto duplicate = integration.routeRawHidEdge(endpoint,
                                                           control, true, 12);
        QVERIFY(!duplicate.accepted);
        const auto release = integration.routeRawHidEdge(endpoint,
                                                         control, false, 40);
        QVERIFY(release.accepted);
    }

    void rawHidRemovalResetsStateAndEvictsAttachment()
    {
        ProviderIntegration integration;
        const QString identity = QStringLiteral("3537:1004");
        const QString control = ControlId::rawHidUsage(identity, 0x09, 0x15);
        QVERIFY(integration.routeRawHidEdge(identity, control, true, 10).accepted);

        QCOMPARE(integration.removeRawHid(identity), QStringList{control});
        QVERIFY(integration.registry()
                    .logicalIdFor(ControllerProvider::RawHid, identity).isEmpty());

        // Re-observation starts a clean generation: the first press is not a
        // duplicate of the held edge from before unplug.
        const auto replug = integration.routeRawHidEdge(identity, control, true, 1000);
        QVERIFY(replug.accepted);
        QVERIFY(!replug.duplicate);
    }

    void weakIdentityUpgradesToDeterministicStrongId()
    {
        ProviderIntegration integration;
        const QString providerId = QStringLiteral("sony.live.1");
        const QString weak = integration.observeLegacy(
            ControllerProvider::SonyRaw, providerId, QStringLiteral("054c:0ce6"),
            QStringLiteral("DualSense"), sonyCapabilities());
        QVERIFY(integration.routeLegacySystemEdge(
                    ControllerProvider::SonyRaw, providerId,
                    ControlId::Capture, true, 10).accepted);

        QStringList safeReleases;
        const QString strong = integration.observeLegacy(
            ControllerProvider::SonyRaw, providerId, QStringLiteral("054c:0ce6"),
            QStringLiteral("DualSense"), sonyCapabilities(), &safeReleases,
            QStringLiteral("hid.endpoint.dualsense-a"));

        QVERIFY(strong != weak);
        QCOMPARE(safeReleases, QStringList{ControlId::Capture});
        QCOMPARE(integration.registry().logicalIdFor(
                     ControllerProvider::SonyRaw, providerId), strong);
        const auto mirroredAfterRekey = integration.routeLegacySystemEdge(
            ControllerProvider::SonyRaw, providerId, ControlId::Capture, true, 20);
        QVERIFY(mirroredAfterRekey.accepted);
        QVERIFY(integration.routeLegacySystemEdge(
                    ControllerProvider::SonyRaw, providerId,
                    ControlId::Capture, false, 30).accepted);
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

    void productionLabelsCarryEnumClassification()
    {
        const auto face = GameInputLabelMap::describe(GI::GameInputLabelXboxA);
        QCOMPARE(face.rawLabel, qint32(GI::GameInputLabelXboxA));
        QCOMPARE(face.normalizedLabel, QStringLiteral("A"));
        QCOMPARE(face.classification, GameInputButtonClassification::Standard);

        const auto share = GameInputLabelMap::describe(GI::GameInputLabelShare);
        QCOMPARE(share.rawLabel, qint32(GI::GameInputLabelShare));
        QCOMPARE(share.classification, GameInputButtonClassification::System);

        const auto macro = GameInputLabelMap::describe(GI::GameInputLabelLetterM);
        QCOMPARE(macro.rawLabel, qint32(GI::GameInputLabelLetterM));
        QCOMPARE(macro.normalizedLabel, QStringLiteral("M"));
        QCOMPARE(macro.classification, GameInputButtonClassification::Extra);
    }

};

QTEST_GUILESS_MAIN(ProviderIntegrationTest)
#include "tst_providerintegration.moc"
