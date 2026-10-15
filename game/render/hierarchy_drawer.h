// The world pass's hierarchical model drawer 0x8002AC40, ported: each joint's offset is turned by the colour
// matrix into the translation its quads are projected with, and a quad inside the clip rectangle and depth range
// becomes one GT4 packet. One body serves the guest override and the render of a saved call.
#pragma once

#include "emit_memory.h"
#include "render/memory_range.h"
#include "state_producer.h"

#include <array>
#include <cstdint>
#include <vector>

class Core;

namespace ts2 {

// a0 the model, a1 the matrix (its rotation loads the colour matrix, +0x14 takes each joint's translation), a2 the
// model's position (the background colour), a3 the joint lift scale, and the stack argument the depth bias.
struct HierarchyDrawCall {
  std::array<std::uint32_t, 5> args{};
  // v1 at entry, what the body hands back when it reaches no quad.
  std::uint32_t v1 = 0;
};

struct HierarchyDrawResult {
  std::uint32_t v0 = 0; // 1 when no quad was drawn
  std::uint32_t v1 = 0;
};

// What a call reads from guest memory and the most packet bytes it can write.
struct HierarchyDrawInputs {
  std::vector<MemoryRange> ranges;
  std::uint32_t packetBytes = 0;
};

class HierarchyDrawer {
public:
  inline static constexpr std::uint32_t kEntry = 0x8002AC40u;
  // 0x8002622C holds the world object record it is drawing in s7 across the call (lw at 0x800262B8).
  inline static constexpr std::uint32_t kCallerObjectRegister = 23u;
  inline static constexpr std::uint32_t kModelRange = 0; // inputs() lists the model first
  inline static constexpr std::uint32_t kMatrixRange = 1;
  inline static constexpr std::uint32_t kPositionRange = 2;
  inline static constexpr std::uint32_t kMatrixBytes = 0x20u;
  inline static constexpr std::uint32_t kPositionBytes = 12u;

  // The guest override and its state render.
  static void install(Core &core, psx::present::StateProducerTable &renders);

  static HierarchyDrawResult run(const psx::present::EmitMemory &memory, const HierarchyDrawCall &call);

  // Read before `run`, so the call can be run again over host copies.
  static HierarchyDrawInputs inputs(const psx::present::EmitMemory &memory, const HierarchyDrawCall &call);
};

} // namespace ts2
