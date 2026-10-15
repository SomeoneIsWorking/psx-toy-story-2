// title_game.h — a Game around ToyStory2Runtime, for a test of one title owner.
#pragma once

#include "core.h"
#include "game.h"
#include "runtime/toystory2_runtime.h"
#include "typed_config.h"

#include <memory>

namespace ts2::test {

class TitleGame {
public:
  TitleGame() : game_(std::make_unique<Game>(runtime_, psx::config::Config{})) {}

  Game &game() {
    return *game_;
  }
  Core &core() {
    return game_->core;
  }
  ToyStory2Runtime &runtime() {
    return runtime_;
  }

  // Activates a resident image over the executable's text, so image-scoped overrides can register.
  void activateResident() {
    game_->core.imageCatalog().activate("resident-test", {0x10000u, 0xA1800u}, 0x5453325245534944ull);
  }

  // What the product does at boot once the executable is admitted.
  void registerOverrides() {
    activateResident();
    runtime_.registerOverrides(*game_);
  }

private:
  ToyStory2Runtime runtime_;
  std::unique_ptr<Game> game_;
};

} // namespace ts2::test
