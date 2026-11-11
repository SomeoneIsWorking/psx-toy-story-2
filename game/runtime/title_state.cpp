#include "runtime/title_state.h"

#include "core.h"
#include "frame/frame_driver.h"
#include "game.h"

namespace ts2 {

bool TitleState::save(psx::platform::BlobWriter &, std::string &) const {
  return true;
}

bool TitleState::load(psx::platform::BlobReader &, std::string &) {
  return true;
}

void TitleState::restored(Core &core) {
  if (auto *driver = core.game->frameDriverIf<FrameDriver>()) {
    driver->drawReplay().forget();
  }
}

} // namespace ts2
