#include "debug/dev_warp.h"

#include "core.h"
#include "execution/guest_execution.h"
#include "facts/guest_facts.h"

#include <lucent/log.h>

#include <array>
#include <utility>

namespace ts2 {
namespace {

constexpr uint32_t kMainLoopEntry = 0x8007A9E8u;

} // namespace

bool DevWarp::warpAllowed() const {
  return outerLoop_.phase == OuterLoopPhase::resident && !outerLoop_.warpLevel &&
         core_.mem_r16(facts::kLoopExitReason) == 0;
}

std::string DevWarp::arm(std::uint32_t level) {
  armed_ = static_cast<uint16_t>(level);
  return lucent::format("ok: warp armed for level {}", level);
}

void DevWarp::applyArmed(uint32_t frame) {
  const std::optional<uint16_t> level = std::exchange(armed_, std::nullopt);
  if (!level) {
    return;
  }
  if (!warpAllowed()) {
    lucent::error("warp", "level {} dropped at f{}: no level is running", *level, frame);
    return;
  }
  core_.mem_w16(facts::kLoopExitReason, kLeaveToFrontEnd);
  core_.mem_w16(facts::kExitCountdown, kExitFadeFields);
  const std::array fadeToBlack{0u, 0u, 0u, kFadeSteps};
  callGuestToReturn(core_, {kFadeTo, kMainLoopEntry, fadeToBlack, std::nullopt, "warp exit fade"});
  outerLoop_.warpLevel = level;
  lucent::info("warp", "level {} requested at f{}", *level, frame);
}

} // namespace ts2
