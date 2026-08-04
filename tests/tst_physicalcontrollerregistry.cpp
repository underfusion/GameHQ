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
