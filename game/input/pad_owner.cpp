#include "input/pad_owner.h"

#include "core.h"
#include "execution/guest_execution.h"
#include "native_dispatch.h"
#include "runtime/toystory2_session.h"

#include <cstdlib>
#include <lucent/log.h>

namespace ts2 {
namespace {

// libpad (VBlank-driven) is neither initialized nor polled.
constexpr uint32_t kPadSlot0 = PadOwner::kSlot0Buffer;
constexpr uint32_t kDeferredDisplayRequest = 0x800A10F8u;
constexpr uint32_t kPadEnabled = 0x800A109Cu;         // gp+0x3c4
constexpr uint32_t kPadKind = 0x800A0CF4u;            // gp+0x1c
constexpr uint32_t kAnalogPacket = 0x800A10B0u;       // gp+0x3d8
constexpr uint32_t kActuatorCount = 0x800A14A8u;      // gp+0x7d0
constexpr uint32_t kDisconnectedFields = 0x800A1580u; // gp+0x8a8

void initializeOverride(Core *core) {
  session(*core).pad.initialize(*core);
}

void shutdownOverride(Core *core) {
  session(*core).pad.shutdown(*core);
}

void decodeOverride(Core *core) {
  core->r[2] = session(*core).pad.decode(*core);
}

} // namespace

void PadOwner::initialize(Core &core) {
  // Retail waits fields and negotiates vibration/analog mode; the native producer exposes one digital pad.
  core.mem_w32(kDeferredDisplayRequest, 0);
  core.mem_w32(kPadKind, 0);
  core.mem_w16(kAnalogPacket, 0);
  core.mem_w32(kActuatorCount, 0);
  core.mem_w16(kDisconnectedFields, 0);
  core.mem_w16(kPadEnabled, 1);
}

void PadOwner::shutdown(Core &core) {
  // 0x8003EF78 brackets PadStopCom with four VSync calls. The host Pad outlives title display resets.
  core.mem_w32(kDeferredDisplayRequest, 0);
  core.mem_w16(kPadEnabled, 0);
}

uint16_t PadOwner::decode(Core &core) {
  if (core.mem_r16(kPadEnabled) == 0) {
    return 0;
  }

  if (core.mem_r8(kPadSlot0) == 0xFFu) {
    const int32_t missing = core.mem_r16s(kDisconnectedFields);
    if (missing > 5) {
      core.mem_w32(kPadKind, 3);
      core.mem_w32(kActuatorCount, 0);
      return 0;
    }
    core.mem_w16(kDisconnectedFields, static_cast<uint16_t>(missing + 1));
    return 0;
  }

  core.mem_w16(kDisconnectedFields, 0);
  const uint8_t packetKind = core.mem_r8(kPadSlot0 + 1) & 0xF0u;
  if (packetKind != 0x40u) {
    lucent::error("ts2-pad",
                  "native pad producer emitted unsupported packet kind 0x{:02X} at 0x{:08X}",
                  packetKind,
                  kPadSlot0 + 1);
    std::abort();
  }

  core.mem_w32(kPadKind, 0);
  core.mem_w16(kAnalogPacket, 0);
  return static_cast<uint16_t>(~core.mem_r16(kPadSlot0 + 2));
}

void PadOwner::install(Core &core) {
  psx::cpu::installNativeOverride(core, 0x8003EEF0u, "pad-init", initializeOverride);
  psx::cpu::installNativeOverride(core, 0x8003EF78u, "pad-shutdown", shutdownOverride);
  psx::cpu::installNativeOverride(core, 0x8003AC58u, "digital-pad-decode", decodeOverride);
}

} // namespace ts2
