#pragma once

#include <cstdint>

class Core;

namespace ts2 {

// Native owner for the digital pad: boot, shutdown and packet-decode boundaries that libpad would own.
// The runtime's StockPadBuffers delivers each pad frame into the slot buffers the decoder reads.
class PadOwner {
public:
  // 0x8003EEF0 passes these buffers to PadInitDirect.
  inline static constexpr uint32_t kSlot0Buffer = 0x800CF8A0u;
  inline static constexpr uint32_t kSlot1Buffer = 0x800CF8C8u;

  void install(Core &core);

  void initialize(Core &core);
  void shutdown(Core &core);

  // What the guest's own decoder answers from the published packet: active-low, release `0xFF`.
  uint16_t decode(Core &core);
};

} // namespace ts2
