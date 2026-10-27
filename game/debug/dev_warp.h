#pragma once

#include "frame/outer_loop.h"

#include <cstdint>
#include <optional>
#include <string>

class Core;

namespace ts2 {

// The debug menu's `warp <level>` request. At the next frame boundary it ends the running level the way the
// attract demo leaves its level (exit reason 4 and the play loop's 46-field fade out), and the outer loop then
// starts level cursor <level> on the player leg through the level start 0x8007BEC4. Legal while a level runs.
class DevWarp final {
public:
  // Cursor positions of the level table 0x8009DEF8; its 16th entry is the 0xFFFFFFFF terminator.
  inline static constexpr int kLevelCount = 15;
  // FUN_8007B254's exit when the attract demo ends: reason 4, countdown 0x2E, FUN_80077598(0, 0, 0, 6).
  inline static constexpr uint16_t kLeaveToFrontEnd = 4;
  inline static constexpr uint16_t kExitFadeFields = 0x2E;
  inline static constexpr uint32_t kFadeTo = 0x80077598u;
  inline static constexpr uint32_t kFadeSteps = 6;

  DevWarp(Core &core, OuterLoopState &outerLoop) : core_(core), outerLoop_(outerLoop) {}

  // A level's play loop is running and nothing has asked it to end.
  bool warpAllowed() const;

  // Arms the request; the returned text answers the client. The debug menu checked the range and warpAllowed().
  std::string arm(std::uint32_t level);
  // Ends the running level for the armed request at a frame boundary.
  void applyArmed(uint32_t frame);

private:
  Core &core_;
  OuterLoopState &outerLoop_;
  std::optional<uint16_t> armed_;
};

} // namespace ts2
