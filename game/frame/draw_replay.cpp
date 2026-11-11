#include "frame/draw_replay.h"

#include "core.h"
#include "execution_exit.h"
#include "facts/guest_facts.h"
#include "frame/frame_driver.h"
#include "game.h"
#include "game_runtime.h"
#include "native_dispatch.h"

namespace ts2 {
namespace {

void drawMode(Core *core) {
  if (auto *driver = core->game->frameDriverIf<FrameDriver>()) {
    driver->drawReplay().beforeDraw(*core);
  }
  psx::cpu::callOriginalToReturn(
      *core, facts::kDrawModePacket, psx::cpu::ExecutionBudget::currentTurn(*core), "draw section");
}

} // namespace

DrawReplay::DrawReplay(Core &core) : motion_(core), frame_(section_, motion_.tracks()) {}

bool replaying(Core &core) {
  const psx::present::ReplayFrame *replay = core.runtime.replayFrame(core);
  return replay != nullptr && replay->replaying().has_value();
}

void DrawReplay::install(Core &core) {
  psx::cpu::installNativeOverride(core, facts::kDrawModePacket, "draw-section", &drawMode);
}

void DrawReplay::beginUpdate(bool orbit) {
  if (orbit != orbit_) {
    frame_.forget();
  }
  orbit_ = orbit;
  section_.select(orbit);
  sectionPending_ = true;
}

void DrawReplay::beforeDraw(Core &core) {
  // The level start's loading card and the front end link the same packet outside a resident update.
  if (!sectionPending_) {
    return;
  }
  sectionPending_ = false;
  drew_ = true;
  frame_.beforeDraw(core);
}

void DrawReplay::endFrame() {
  if (!drew_) {
    frame_.forget();
  }
  drew_ = false;
  sectionPending_ = false;
}

void DrawReplay::forget() {
  frame_.forget();
}

} // namespace ts2
