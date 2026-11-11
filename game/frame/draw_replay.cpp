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

// The world pass's profiler mark (GetRCnt into a table).
constexpr std::uint32_t kProfilerMark = 0x8003E938u;

void drawMode(Core *core) {
  if (auto *driver = core->game->frameDriverIf<FrameDriver>()) {
    driver->drawReplay().beforeDraw(*core);
  }
  psx::cpu::callOriginalToReturn(
      *core, facts::kDrawModePacket, psx::cpu::ExecutionBudget::currentTurn(*core), "draw section");
}

bool replaying(Core &core) {
  const psx::present::ReplayFrame *replay = core.runtime.replayFrame(core);
  return replay != nullptr && replay->replaying().has_value();
}

// DrawSync(0) waits on the GPU a replay never drives; the real frame's wait returns 0 as soon as it asks.
void drawSync(Core *core) {
  if (replaying(*core)) {
    core->r[2] = 0;
    return;
  }
  psx::cpu::callOriginalToReturn(*core, facts::kDrawSync, psx::cpu::ExecutionBudget::currentTurn(*core), "draw sync");
}

// The profiler mark stores a root-counter read into a table nothing draws; a replay's stores are put back anyway.
void profilerMark(Core *core) {
  if (replaying(*core)) {
    return;
  }
  psx::cpu::callOriginalToReturn(*core, kProfilerMark, psx::cpu::ExecutionBudget::currentTurn(*core), "profiler mark");
}

} // namespace

DrawReplay::DrawReplay(Core &core) : motion_(core), frame_(section_, motion_.tracks()) {}

void DrawReplay::install(Core &core) {
  psx::cpu::installNativeOverride(core, facts::kDrawModePacket, "draw-section", &drawMode);
  psx::cpu::installNativeOverride(core, facts::kDrawSync, "draw-sync", &drawSync);
  psx::cpu::installNativeOverride(core, kProfilerMark, "profiler-mark", &profilerMark);
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
