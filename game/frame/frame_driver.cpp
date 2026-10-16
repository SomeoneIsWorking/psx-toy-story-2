#include "frame/frame_driver.h"

#include "core.h"
#include "frame/resident_frame.h"
#include "game.h"

#include <memory>

namespace ts2 {

void FrameDriver::stepFrame(Core &core, uint32_t frame) {
  warp_.applyArmed(frame);
  CoreFrameBoundary boundary(core, state_);
  stepResidentFrame(boundary, frame);
}

std::unique_ptr<psx::game::FrameDriver> createFrameDriver(Game &game) {
  return std::make_unique<FrameDriver>(game.core);
}

} // namespace ts2
