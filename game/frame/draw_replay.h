// draw_replay.h — Toy Story 2's gameplay in-betweens: the resident update's draw section replayed with its
// moving inputs blended.
#pragma once

#include "frame/draw_motion.h"
#include "frame/draw_section.h"
#include "replay_frame.h"

class Core;

namespace ts2 {

class DrawReplay {
public:
  explicit DrawReplay(Core &core);

  // The overrides of the section's first step and of the profiler mark the section calls.
  static void install(Core &core);

  // The resident update 0x8007B254, or the orbit 0x8007B850 when `orbit`, is about to run; a switch between them
  // breaks the pairing.
  void beginUpdate(bool orbit);
  // The section's first step is about to run for real, inside or outside a resident update.
  void beforeDraw(Core &core);
  // The logic frame ended; one that did not draw the section breaks the pairing.
  void endFrame();
  // The next draw pairs with nothing (a savestate restore).
  void forget();

  [[nodiscard]] psx::present::ReplayFrame &frame() {
    return frame_;
  }

private:
  DrawSection section_;
  DrawMotion motion_;
  psx::present::ReplayFrame frame_;
  // A resident update is running and has not drawn yet.
  bool sectionPending_ = false;
  bool orbit_ = false;
  bool drew_ = false;
};

} // namespace ts2
