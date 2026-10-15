#pragma once

#include "game_runtime.h"

#include <memory>

class Game;

namespace ts2 {

std::unique_ptr<psx::game::FrameDriver> createFrameDriver(Game &game);

} // namespace ts2
