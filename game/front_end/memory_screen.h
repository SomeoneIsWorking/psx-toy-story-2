#pragma once

#include <cstdint>

class Core;

namespace ts2 {

// MEMORY 0x800D9704(id): one still screen held by a 600-field countdown, one field per host turn; Start ends it
// through its own fade at any time, where retail first waits out 300 fields.
class MemoryScreen {
public:
  // 0x800D5D20 is a slot FMV.BIN also occupies, so install only once the MEMORY image is resident.
  void install(Core &core);

  void enter(Core &core);

private:
  enum class Phase : std::uint8_t { idle, atBarrier };

  void yieldField(Core &core);

  Phase phase_ = Phase::idle;
  std::int32_t remaining_ = 0;
  std::int32_t startFrom_ = 0;
  std::uint32_t result_ = 0;
};

} // namespace ts2
