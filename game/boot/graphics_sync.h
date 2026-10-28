#pragma once

#include <cstdint>

class Core;

namespace ts2 {

// Also installs guest VBlank (0x80039D60), which nothing else dispatches.
class GraphicsSync {
public:
  void install(Core &core);
};

// 0x8003FA68's work once `elapsed` fields have passed: publish them, then run the field's deferred display 0x80021028.
void publishFieldBarrier(Core &core, std::uint32_t elapsed);

} // namespace ts2
