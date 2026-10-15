#include "runtime/session.h"

#include "core.h"
#include "game.h"

#include <cstdlib>
#include <lucent/log.h>

namespace ts2 {

Session &session(Core &core) {
  if (core.game == nullptr || !core.game->sessionState) {
    lucent::error("ts2-runtime", "Session is absent from the Game");
    std::abort();
  }
  return static_cast<Session &>(*core.game->sessionState);
}

} // namespace ts2
