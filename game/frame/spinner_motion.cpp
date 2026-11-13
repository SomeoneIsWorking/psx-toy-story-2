#include "frame/spinner_motion.h"

#include "core.h"
#include "frame/frame_cut.h"
#include "guest_redraw.h"

#include <algorithm>
#include <utility>

namespace ts2 {
namespace {

using psx::present::Motion;

// The effect pool: a u16 count, then records of 0x10 from +4; a record whose byte +0xC is at least 0x30 names
// AreaObjects entry +0xC, whose group +0x10 holds the instance at +0x14.
constexpr std::uint32_t kEffectPool = 0x800BB3CCu;
constexpr std::uint32_t kPoolRecords = 0x04u;
constexpr std::uint32_t kRecordBytes = 0x10u;
constexpr std::uint32_t kRecordObject = 0x0Cu;
constexpr std::uint8_t kFirstObject = 0x30u;
constexpr std::uint32_t kEntryGroup = 0x10u;
constexpr std::uint32_t kGroupInstance = 0x14u;
// Angles s16 +0x0C/+0x0E/+0x10; +0x12 is the x scale.
constexpr std::uint32_t kTurnXY = 0x0Cu;
constexpr std::uint32_t kTurnZ = 0x10u;

} // namespace

void SpinnerMotion::sample(Core &core) {
  std::swap(from_, to_);
  to_.clear();
  const std::uint32_t pool = core.mem_r32(kEffectPool);
  const std::uint32_t objects = core.mem_r32(FrameCut::kAreaObjects);
  if (pool == 0u || objects == 0u) {
    return;
  }
  const std::uint32_t count = core.mem_r16(pool);
  for (std::uint32_t record = 0; record < count; ++record) {
    const std::uint8_t object = core.mem_r8(pool + kPoolRecords + record * kRecordBytes + kRecordObject);
    if (object < kFirstObject) {
      continue;
    }
    const std::uint32_t entry = core.mem_r32(objects + 4u + object * 4u);
    if (entry == 0u) {
      continue;
    }
    const std::uint32_t instance = core.mem_r32(core.mem_r32(entry + kEntryGroup) + kGroupInstance);
    to_.push_back({.record = record,
                   .instance = instance,
                   .turnXY = core.mem_r32(instance + kTurnXY),
                   .turnZ = core.mem_r32(instance + kTurnZ)});
  }
}

psx::present::MotionCount SpinnerMotion::write(psx::present::GuestRedraw &redraw, float t) const {
  psx::present::MotionCount count{.drawn = to_.size()};
  for (const Spinner &now : to_) {
    const auto before = std::ranges::find_if(from_, [&](const Spinner &s) {
      return s.record == now.record && s.instance == now.instance;
    });
    if (before == from_.end()) {
      ++count.unpaired;
      continue;
    }
    redraw.writeWord(now.instance + kTurnXY,
                     psx::present::blendedWord(Motion::Angle12x2, before->turnXY, now.turnXY, t));
    redraw.writeWord(now.instance + kTurnZ, psx::present::blendedWord(Motion::Angle12Low, before->turnZ, now.turnZ, t));
  }
  return count;
}

} // namespace ts2
