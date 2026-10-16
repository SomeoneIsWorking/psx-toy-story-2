#include "debug/dev_warp.h"

#include "core.h"
#include "execution/guest_execution.h"
#include "facts/guest_facts.h"

#include <lucent/log.h>

#include <array>
#include <charconv>
#include <string_view>
#include <utility>

namespace ts2 {
namespace {

constexpr uint32_t kMainLoopEntry = 0x8007A9E8u;

std::optional<uint16_t> parseLevel(std::string_view line) {
  constexpr std::string_view kCommand = "warp ";
  if (!line.starts_with(kCommand)) {
    return std::nullopt;
  }
  line.remove_prefix(kCommand.size());
  while (line.ends_with(' ') || line.ends_with('\n') || line.ends_with('\r')) {
    line.remove_suffix(1);
  }
  uint16_t level = 0;
  const auto [end, error] = std::from_chars(line.data(), line.data() + line.size(), level);
  if (error != std::errc{} || end != line.data() + line.size()) {
    return std::nullopt;
  }
  return level;
}

} // namespace

bool DevWarp::warpAllowed() const {
  return outerLoop_.phase == OuterLoopPhase::resident && !outerLoop_.warpLevel &&
         core_.mem_r16(facts::kLoopExitReason) == 0;
}

std::string DevWarp::arm(const char *line) {
  const std::optional<uint16_t> level = parseLevel(line);
  if (!level) {
    return "usage: warp <level>";
  }
  if (*level >= kLevelCount) {
    return lucent::format("refused: level {} is out of range (0..{})", *level, kLevelCount - 1);
  }
  if (!warpAllowed()) {
    return "refused: a warp is legal only while a level is running";
  }
  armed_ = level;
  return lucent::format("ok: warp armed for level {}", *level);
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
