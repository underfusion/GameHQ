#include "input/PhysicalControllerRegistry.h"

#include <QtTest>

using namespace ModernInput;

class PhysicalControllerRegistryTest : public QObject
{
    Q_OBJECT

private slots:
    void strongIdentityMergesCapabilities()
    {
        PhysicalControllerRegistry registry;
        ProviderObservation sony{ControllerProvider::SonyRaw, QStringLiteral("raw-1")};
        sony.containerId = QStringLiteral("container-a");
        sony.capabilities = ControllerCapability::StandardControls;
        ProviderObservation gameInput{ControllerProvider::GameInput, QStringLiteral("gi-1")};
        gameInput.containerId = sony.containerId;
        gameInput.capabilities = ControllerCapability::SystemShare
            | ControllerCapability::Guide | ControllerCapability::ExtraControls;

        const QString first = registry.observe(sony);
        const QString second = registry.observe(gameInput);

        QCOMPARE(first, second);
        const auto* logical = registry.controller(first);
        QVERIFY(logical);
        QCOMPARE(logical->providers.size(), 2);
        QVERIFY(logical->capabilities().testFlag(ControllerCapability::SystemShare));
        QCOMPARE(registry.preferredProvider(first, ControllerCapability::StandardControls),
                 ControllerProvider::SonyRaw);
        QCOMPARE(registry.preferredProvider(first, ControllerCapability::SystemShare),
                 ControllerProvider::GameInput);
    }

    void weakVidPidNeverMergesIdenticalModels()
    {
        PhysicalControllerRegistry registry;
        ProviderObservation first{ControllerProvider::XInput, QStringLiteral("slot-0")};
        first.vendorId = 0x1234;
        first.productId = 0x5678;
        ProviderObservation second = first;
        second.providerDeviceId = QStringLiteral("slot-1");

        const QString firstId = registry.observe(first);
        const QString secondId = registry.observe(second);

        QVERIFY(firstId != secondId);
        QCOMPARE(registry.controllers().size(), 2);
        QCOMPARE(registry.controller(firstId)->confidence, IdentityConfidence::Weak);
    }

    void correlatedIdentityRejectsAmbiguousTopology()
    {
        PhysicalControllerRegistry registry;
        ProviderObservation first{ControllerProvider::XInput, QStringLiteral("x-1")};
        first.topologyRoot = QStringLiteral("receiver-root");
        first.containerId = QStringLiteral("container-1");
        ProviderObservation second{ControllerProvider::XInput, QStringLiteral("x-2")};
        second.topologyRoot = first.topologyRoot;
        second.containerId = QStringLiteral("container-2");
        const QString firstId = registry.observe(first);
        const QString secondId = registry.observe(second);
        QVERIFY(firstId != secondId);

        ProviderObservation gameInput{ControllerProvider::GameInput, QStringLiteral("gi")};
        gameInput.topologyRoot = first.topologyRoot;
        const QString thirdId = registry.observe(gameInput);
        QVERIFY(thirdId != firstId);
        QVERIFY(thirdId != secondId);
    }

    void reversedObservationOrderYieldsSameStrongLogicalId()
    {
        // The logical ID is persisted (binding profiles, controller_layouts):
        // it must not depend on which provider observed the device first.
        ProviderObservation sony{ControllerProvider::SonyRaw, QStringLiteral("raw-1")};
        sony.appLocalDeviceId = QStringLiteral("app-local-1");
        ProviderObservation gameInput{ControllerProvider::GameInput, QStringLiteral("gi-1")};
        gameInput.appLocalDeviceId = sony.appLocalDeviceId;

        PhysicalControllerRegistry first;
        const QString firstId = first.observe(sony);
        QCOMPARE(first.observe(gameInput), firstId);

        PhysicalControllerRegistry second;
        const QString secondId = second.observe(gameInput);
        QCOMPARE(second.observe(sony), secondId);

        QCOMPARE(firstId, secondId);
    }

    void reversedOrderKeepsTwoDevicesDistinctAndStable()
    {
        ProviderObservation padA{ControllerProvider::GameInput, QStringLiteral("gi-a")};
        padA.appLocalDeviceId = QStringLiteral("app-a");
        ProviderObservation padB{ControllerProvider::GameInput, QStringLiteral("gi-b")};
        padB.appLocalDeviceId = QStringLiteral("app-b");

        PhysicalControllerRegistry first;
        const QString aFirst = first.observe(padA);
        const QString bFirst = first.observe(padB);
        PhysicalControllerRegistry second;
        const QString bSecond = second.observe(padB);
        const QString aSecond = second.observe(padA);

        QVERIFY(aFirst != bFirst);
        QCOMPARE(aFirst, aSecond);
        QCOMPARE(bFirst, bSecond);
    }

    void conflictingStrongIdsInSameContainerNeverMerge()
    {
        // A hub/receiver container can hold several endpoints: matching
        // containers must never override conflicting strong device IDs.
        PhysicalControllerRegistry registry;
        ProviderObservation one{ControllerProvider::GameInput, QStringLiteral("gi-1")};
        one.appLocalDeviceId = QStringLiteral("app-1");
        one.containerId = QStringLiteral("shared-container");
        ProviderObservation two{ControllerProvider::GameInput, QStringLiteral("gi-2")};
        two.appLocalDeviceId = QStringLiteral("app-2");
        two.containerId = one.containerId;

        const QString firstId = registry.observe(one);
        const QString secondId = registry.observe(two);
        QVERIFY(firstId != secondId);
        QCOMPARE(registry.controllers().size(), 2);
    }

    void reObservationRefreshesAttachmentCapabilities()
    {
        PhysicalControllerRegistry registry;
        ProviderObservation gameInput{ControllerProvider::GameInput, QStringLiteral("gi-1")};
        gameInput.appLocalDeviceId = QStringLiteral("app-1");
        gameInput.capabilities = ControllerCapability::StandardControls;
        const QString id = registry.observe(gameInput);
        QVERIFY(!registry.controller(id)->capabilities()
                     .testFlag(ControllerCapability::SystemShare));

        // CapabilityChanged re-observes the same attachment with new flags.
        gameInput.capabilities = ControllerCapability::StandardControls
            | ControllerCapability::SystemShare;
        QCOMPARE(registry.observe(gameInput), id);
        QCOMPARE(registry.controller(id)->providers.size(), 1);
        QVERIFY(registry.controller(id)->capabilities()
                    .testFlag(ControllerCapability::SystemShare));
    }

    void providerRemovalKeepsRemainingLogicalController()
    {
        PhysicalControllerRegistry registry;
        ProviderObservation one{ControllerProvider::XInput, QStringLiteral("x")};
        one.containerId = QStringLiteral("same");
        ProviderObservation two{ControllerProvider::GameInput, QStringLiteral("g")};
        two.containerId = one.containerId;
        const QString id = registry.observe(one);
        registry.observe(two);

        QVERIFY(registry.removeProvider(ControllerProvider::XInput, QStringLiteral("x")));
        QVERIFY(registry.controller(id));
        QCOMPARE(registry.controller(id)->providers.size(), 1);
    }
};

QTEST_MAIN(PhysicalControllerRegistryTest)
#include "tst_physicalcontrollerregistry.moc"
