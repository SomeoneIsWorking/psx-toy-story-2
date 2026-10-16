#pragma once

#include "debug/dev_warp.h"
#include "frame/frame_boundary.h"
#include "game_runtime.h"

#include <cstdint>
#include <memory>

class Core;
class Game;

namespace ts2 {

// One stepResidentFrame per step; an armed dev warp is applied first, between frames.
class FrameDriver final : public psx::game::FrameDriver {
public:
  explicit FrameDriver(Core &core) : state_(core), warp_(core, state_.outerLoop()) {}

  void stepFrame(Core &core, uint32_t frame) override;

  DevWarp &devWarp() {
    return warp_;
  }

private:
  FrameCallState state_;
  DevWarp warp_;
};

std::unique_ptr<psx::game::FrameDriver> createFrameDriver(Game &game);

} // namespace ts2
