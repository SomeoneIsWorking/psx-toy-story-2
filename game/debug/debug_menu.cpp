#include "debug/debug_menu.h"

#include "core.h"
#include "debug/dev_warp.h"
#include "frame/frame_driver.h"
#include "game.h"

namespace ts2 {
namespace {

DevWarp *devWarp(Core &core) {
  auto *driver = core.game->frameDriverIf<FrameDriver>();
  return driver != nullptr ? &driver->devWarp() : nullptr;
}

} // namespace

DebugMenu::DebugMenu() {
  using psx::game::DebugCall;
  using psx::game::DebugParam;
  section("Level Warp");
  verb({
      .name = "warp",
      .summary = "Start that level",
      .params = {DebugParam::integer("level", 0, DevWarp::kLevelCount - 1)},
      .run =
          [](const DebugCall &call) {
            call.reply = devWarp(call.core)->arm(call.values.u32(0));
          },
      .refusal =
          [](Core &core) {
            const DevWarp *warp = devWarp(core);
            return warp != nullptr && warp->warpAllowed() ? std::string()
                                                          : "a warp is legal only while a level is running";
          },
  });
}

} // namespace ts2
