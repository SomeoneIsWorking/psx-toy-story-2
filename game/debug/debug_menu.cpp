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

std::optional<psx::game::DecompImage>
DebugMenu::decompImage(Core &core, std::string_view imageName, bool resident) const {
  if (resident) {
    return TitleDebugMenu::decompImage(core, imageName, resident);
  }
  // The catalog names a module by its disc path: LEVELnn/LEVEL.BIN, LEVELnn/LEVEL1.BIN, BITS/MEMORY.BIN, FMV/FMV.BIN.
  const std::size_t slash = imageName.find('/');
  if (slash == std::string_view::npos) {
    return std::nullopt;
  }
  const std::string_view directory = imageName.substr(0, slash);
  const std::string_view file = imageName.substr(slash + 1);
  std::string manifest;
  if (directory.starts_with("LEVEL") && (file == "LEVEL.BIN" || file == "LEVEL1.BIN")) {
    manifest = "toystory2_level" + std::string(directory.substr(5)) + (file == "LEVEL1.BIN" ? "1" : "");
  } else if (imageName == "BITS/MEMORY.BIN") {
    manifest = "toystory2_memory";
  } else if (imageName == "FMV/FMV.BIN") {
    manifest = "toystory2_fmv";
  } else {
    return std::nullopt;
  }
  return psx::game::DecompImage{manifest, "scratch/flat/" + std::string(directory) + "__" + std::string(file)};
}

} // namespace ts2
