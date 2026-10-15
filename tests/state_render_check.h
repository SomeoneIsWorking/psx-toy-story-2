// state_render_check.h — what a producer test needs to compare a state render with the frame the body drew.
#pragma once

#include "core.h"
#include "execution_exit.h"
#include "facts/guest_facts.h"
#include "frame_state.h"
#include "game_runtime.h"
#include "gp0_primitive_decode.h"
#include "native_dispatch.h"
#include "render/ordering_tables.h"
#include "state_producer.h"
#include "title_game.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace ts2::test {

using Drawn = std::vector<std::pair<psx::present::OtSlot, psx::present::DrawPrimitive>>;

inline constexpr std::uint32_t kReturn = 0x80010100u;
// Bucket 0 of buffer A's ordering table.
inline constexpr std::uint32_t kTableBase = facts::kGraphicsBufferA + facts::kOrderingTableOffset;

inline std::unique_ptr<TitleGame> residentGame() {
  auto game = std::make_unique<TitleGame>();
  game->activateResident();
  return game;
}

// Enters `entry` as a guest call returning to kReturn.
inline bool call(Core &core, std::uint32_t entry) {
  core.r[29] = 0x801FFF00u;
  core.r[31] = kReturn;
  return psx::cpu::dispatchGuest(core, entry, psx::cpu::ExecutionBudget::fromCycles(1000000u)).returned();
}

// A helper that cannot return a CHECK's early exit stops the test where it is.
inline void require(bool condition, const char *what) {
  if (!condition) {
    std::fprintf(stderr, "    REQUIRED: %s\n", what);
    std::abort();
  }
}

template <typename T> T required(std::optional<T> value, const char *what) {
  if (!value) {
    std::fprintf(stderr, "    REQUIRED: %s\n", what);
    std::abort();
  }
  return *value;
}

class Collect final : public psx::present::PrimitiveSink {
public:
  void emit(psx::present::OtSlot slot, const psx::present::DrawPrimitive &primitive) override {
    emitted.emplace_back(slot, primitive);
  }
  Drawn emitted;
};

// What the guest's table holds, bucket by bucket from the last, as the walk decodes it.
inline Drawn guestPrimitives(Core &core) {
  Drawn primitives;
  for (std::uint32_t bucket = facts::kOrderingTableBuckets; bucket-- > 0;) {
    std::uint32_t packet = core.mem_r32(kTableBase + bucket * 4u) & 0x00FFFFFFu;
    while (packet != 0u && packet != 0x00FFFFFFu) {
      const std::uint32_t tag = core.mem_r32(packet);
      std::vector<std::uint32_t> words;
      for (std::uint32_t word = 0; word != (tag >> 24); ++word) {
        words.push_back(core.mem_r32(packet + 4u + word * 4u));
      }
      primitives.emplace_back(psx::present::OtSlot{OrderingTables::kTableId, bucket},
                              required(psx::gpu::decodePacketPrimitive(words), "a linked packet decodes"));
      packet = tag & 0x00FFFFFFu;
    }
  }
  return primitives;
}

// The state the packet at `packet` names, collected as the record that drew it would.
inline psx::present::FrameState collectedState(Core &core, std::uint32_t packet) {
  psx::present::FrameRecord record(1, true);
  psx::present::DrawPrimitive primitive;
  primitive.key = core.emission.keyFor(packet);
  record.append(primitive);
  return core.frameStates.collect(record);
}

// `producer`'s render `t` of the way from `from` to `to`.
inline Collect renderState(
    Core &core, std::uint32_t producer, std::span<const std::byte> from, std::span<const std::byte> to, float t) {
  const psx::present::StateProducer *render = core.runtime.stateProducer(core, producer);
  require(render != nullptr, "the producer has a render");
  Collect sink;
  render->render({from, to, t}, sink);
  return sink;
}

inline bool samePrimitives(const Collect &sink, const Drawn &expected) {
  if (sink.emitted.size() != expected.size() || expected.empty()) {
    return false;
  }
  for (std::size_t i = 0; i != expected.size(); ++i) {
    if (!(sink.emitted[i].first == expected[i].first) || !(sink.emitted[i].second == expected[i].second)) {
      return false;
    }
  }
  return true;
}

} // namespace ts2::test
