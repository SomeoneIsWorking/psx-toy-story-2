// draw_motion.h — the guest words the draw section reads that move between resident updates: the camera the
// update's 0x8002C848 leaves in the scratchpad, the player and the pooled actors.
#pragma once

#include "motion_set.h"

#include <array>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <unordered_set>

class Core;

namespace ts2 {

// An actor record as 0x8002518C reads it: position s32 +0x00/+0x04/+0x08, angles s16 +0x0C/+0x0E/+0x10,
// animation indices s16 +0x12/+0x16 and their 16.16 frame words +0x18/+0x1C. 0x800426E0 poses the model between
// a frame and the next by the low half, the last frame towards frame 0, so a frame word turns round the
// animation's frame count; it blends only while its animation index holds.
class ActorMotion final : public psx::present::MotionRule {
public:
  // An animation layer: its frame word, and the word whose high half holds its animation index.
  struct PoseLayer {
    std::size_t frame = 0;
    std::size_t animation = 0;
  };

  explicit ActorMotion(Core &core) : core_(core) {}
  void blend(const Samples &samples, float t, std::span<std::uint32_t> out) const override;

private:
  // The frame count of `layer`'s animation of the record's model, or 0 when it has none.
  [[nodiscard]] std::uint32_t frameCount(std::span<const std::uint32_t> words, const PoseLayer &layer) const;

  Core &core_;
};

// The actor pool's live records: those on the active list 0x800B18A8 the draw list 0x800A4298 is built from, in
// the life (ActorIncarnations) the previous sample saw.
class ActorLiveness final : public psx::present::MotionLiveness {
public:
  void sample(Core &core) override;
  [[nodiscard]] bool live(std::uint32_t record) const override;

private:
  std::unordered_set<std::uint32_t> listed_;
  std::unordered_map<std::uint32_t, std::uint32_t> generations_;
  std::unordered_set<std::uint32_t> reborn_;
};

// A part record of the shared table 0x800A9618 (0x6C bytes) as 0x8002518C reads it: its offset s32 +0x00/+0x04/
// +0x08 and its rotation, nine s16 from +0x58, whose last word's high half is another field and steps.
class PartMotion final : public psx::present::MotionRule {
public:
  void blend(const Samples &samples, float t, std::span<std::uint32_t> out) const override;
};

// The part records the update posed: those of a model the draw list draws once, for an actor with no animation
// for 0x800426E0 to pose in the draw.
class PartLiveness final : public psx::present::MotionLiveness {
public:
  void sample(Core &core) override;
  [[nodiscard]] bool live(std::uint32_t record) const override;

private:
  std::unordered_set<std::uint32_t> posed_;
};

class DrawMotion {
public:
  explicit DrawMotion(Core &core);
  DrawMotion(const DrawMotion &) = delete;
  DrawMotion &operator=(const DrawMotion &) = delete;
  DrawMotion(DrawMotion &&) = delete;
  DrawMotion &operator=(DrawMotion &&) = delete;

  [[nodiscard]] std::span<const psx::present::MotionTrack> tracks() const {
    return tracks_;
  }

private:
  static constexpr std::size_t kTracks = 6;

  psx::present::TransformRule transform_;
  ActorMotion actor_;
  ActorLiveness actors_;
  PartMotion part_;
  PartLiveness parts_;
  std::array<psx::present::MotionTrack, kTracks> tracks_;
};

} // namespace ts2
