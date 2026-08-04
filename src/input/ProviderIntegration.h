#pragma once

#include "input/CapabilityEventRouter.h"
#include "input/PhysicalControllerRegistry.h"

#include <QHash>
#include <QSet>
#include <QString>

namespace ModernInput {

// One registry + one capability router shared by every real input provider
// (Sony Raw Input, GameInput, XInput, WinMM, selective Raw HID). This is the
// t25 integration seam: providers report physical identity and lifecycle
// here, and every system-capable edge (Capture/Guide) and raw-HID edge is
// routed through the shared CapabilityEventRouter so one physical button can
// never fire twice through two provider pipelines.
//
// Correlation model: legacy providers carry no GameInput appLocalDeviceId, so
// they correlate through topologyRoot = the lowercase "vvvv:pppp" hardware
// fingerprint. PhysicalControllerRegistry accepts a topology correlation only
// when it identifies exactly ONE logical controller — two identical pads stay
// distinct (and therefore keep GameInput routing shadowed, which is the safe
// answer when identity is ambiguous).
//
// Threading: main (Qt GUI) thread only, like the registry and router it owns.
// GameInput callbacks never reach this object directly — they cross the
// mutex-protected GameInputEventQueue first.
class ProviderIntegration
{
public:
    ProviderIntegration() : m_capabilityRouter(&m_registry) {}

    PhysicalControllerRegistry& registry() { return m_registry; }
    const PhysicalControllerRegistry& registry() const { return m_registry; }
    CapabilityEventRouter& capabilityRouter() { return m_capabilityRouter; }

    // A legacy backend became active. `fingerprint` is the backend's stable
    // hardware identity ("054c:0ce6"); it doubles as the topology correlation
    // root. Returns the logical controller id the attachment landed on.
    QString observeLegacy(ControllerProvider provider, const QString& providerDeviceId,
                          const QString& fingerprint, const QString& displayName,
                          ControllerCapabilities capabilities);
    void removeLegacy(ControllerProvider provider, const QString& providerDeviceId);

    // True while ANY legacy provider attachment (Sony Raw, XInput, WinMM) is
    // live. GameInput routing uses this as its safety gate: an edge whose
    // logical controller has no correlated legacy attachment stays shadowed
    // while this is true, because an uncorrelated legacy pad could mirror the
    // same physical control through the legacy dispatch path.
    bool legacyProviderConnected() const { return !m_legacyKeys.isEmpty(); }
    bool hasLegacyAttachment(const QString& logicalId) const;

    // Route a Capture/Guide edge from a legacy provider through the shared
    // dedup. Fails OPEN (accepted, no dedup) if the attachment was never
    // observed: the proven legacy path must never lose input to bookkeeping.
    CapabilityRouteResult routeLegacySystemEdge(ControllerProvider provider,
                                                const QString& providerDeviceId,
                                                const QString& controlId, bool pressed,
                                                quint64 timestamp);

    // Route a selective-Raw-HID edge. Observes the RawHid attachment on first
    // use so the device correlates (by "vvvv:pppp" identity) with any logical
    // controller other providers already reported.
    CapabilityRouteResult routeRawHidEdge(const QString& deviceIdentity,
                                          const QString& controlId, bool pressed,
                                          quint64 timestamp);

    static ControllerCapability systemCapabilityFor(const QString& controlId);

private:
    static QString legacyKey(ControllerProvider provider, const QString& providerDeviceId);

    PhysicalControllerRegistry m_registry;
    CapabilityEventRouter m_capabilityRouter;
    QSet<QString> m_legacyKeys;
    QSet<QString> m_rawHidObserved;
};

} // namespace ModernInput
