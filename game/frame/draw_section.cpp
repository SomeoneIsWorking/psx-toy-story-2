#include "frame/draw_section.h"

#include "core.h"
#include "facts/guest_facts.h"
#include "guest_redraw.h"
#include "ordering_table.h"

#include <lucent/log.h>

#include <array>
#include <span>

namespace ts2 {
namespace {

// The reversed OT's head: its last bucket, ClearOTagR(+0x2B4, 0x57C).
constexpr std::uint32_t kOtHead = facts::kOrderingTableOffset + (facts::kOrderingTableBuckets - 1u) * 4u;

// 0x8003A888's links before DrawOTag: the screen bars, the screen effect of mode 2 and the colour tint.
constexpr std::uint32_t kScreenBars = 0x8003DEC4u;
constexpr std::uint32_t kScreenEffect = 0x8003EFD4u;
constexpr std::uint32_t kTint = 0x8002DC2Cu;
// The orbit update 0x8007B850 links the pause menu after the world pass; 0x8007B254 runs its logic before.
constexpr std::uint32_t kPauseMenu = 0x800742C4u;
// gp+0x588 and gp+0x610 (s16): the display's effect modes; 0x610 == 1 or 0x588 == 1 copy VRAM instead.
constexpr std::uint32_t kEffectMode = 0x800A1260u;
constexpr std::uint32_t kEffectOverride = 0x800A12E8u;

bool step(psx::present::GuestRedraw &redraw, std::uint32_t entry, std::span<const std::uint32_t> args = {}) {
  const bool returned = redraw.dispatch(entry, args);
  lucent::debug("replay", "ts2 section step 0x{:08X}: {}", entry, returned ? "returned" : "refused");
  return returned;
}

} // namespace

std::optional<std::uint32_t> DrawSection::draw(Core &core, psx::present::GuestRedraw &redraw) const {
  // The packet's override samples the motion before the real draw; its original opens the replay.
  if (!redraw.run(facts::kDrawModePacket, {}) || !step(redraw, kWorldPass)) {
    return std::nullopt;
  }
  if (orbit_ && !step(redraw, kPauseMenu)) {
    return std::nullopt;
  }
  const std::uint32_t buffer = core.mem_r32(facts::kDrawBuffer);
  const std::array bars{buffer};
  if (!step(redraw, kScreenBars, bars)) {
    return std::nullopt;
  }
  if (core.mem_r16s(kEffectOverride) != 1 && core.mem_r16s(kEffectMode) == 2 && !step(redraw, kScreenEffect)) {
    return std::nullopt;
  }
  if (!step(redraw, kTint)) {
    return std::nullopt;
  }
  return buffer + kOtHead;
}

bool DrawSection::linked(std::uint32_t head) const {
  const std::uint32_t offset = psx::gpu::mainRamOffsetOf(head);
  return offset == psx::gpu::mainRamOffsetOf(facts::kGraphicsBufferA + kOtHead) ||
         offset == psx::gpu::mainRamOffsetOf(facts::kGraphicsBufferB + kOtHead);
}

} // namespace ts2
