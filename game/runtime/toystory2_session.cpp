#include "runtime/toystory2_session.h"

#include "core.h"
#include "game.h"

#include <cstdlib>
#include <lucent/log.h>

namespace ts2 {

ToyStory2Session &session(Core &core) {
  if (core.game == nullptr || !core.game->sessionState) {
    lucent::error("ts2-runtime", "ToyStory2Session is absent from the Game");
    std::abort();
  }
  return static_cast<ToyStory2Session &>(*core.game->sessionState);
}

} // namespace ts2
