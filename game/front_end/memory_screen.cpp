#include "front_end/memory_screen.h"

#include "boot/graphics_sync.h"
#include "core.h"
#include "execution/guest_execution.h"
#include "execution_control.h"
#include "execution_exit.h"
#include "native_dispatch.h"
#include "runtime/session.h"

#include <array>
#include <cstdlib>
#include <lucent/log.h>

namespace ts2 {
namespace {

constexpr std::uint32_t kScreen = 0x800D9704u;
constexpr std::uint32_t kScreenReset = 0x800D6614u;
constexpr std::uint32_t kShowGraphic = 0x8003D084u;
constexpr std::uint32_t kAdvanceFrame = 0x800775CCu;
constexpr std::uint32_t kDrawGraphic = 0x8003E8B4u;
constexpr std::uint32_t kSubmitFrame = 0x80046A88u;
constexpr std::uint32_t kFadeOut = 0x80077598u;

constexpr std::uint32_t kColdStartFlag = 0x800A1670u; // halfword; nonzero after the first boot
constexpr std::uint32_t kElapsedFields = 0x800A1174u;
constexpr std::uint32_t kPadPressed = 0x800A1480u;
constexpr std::uint32_t kPadPrevious = 0x800A11E4u;
constexpr std::uint16_t kStart = 0x0008u;
constexpr std::uint16_t kDirections = 0xF000u;

constexpr std::int32_t kHoldFields = 600;
constexpr std::int32_t kFirstBootLock = 300;
constexpr std::int32_t kFadeFields = 0x18;

void call(Core &core, std::uint32_t address, const std::array<std::uint32_t, 4> &arguments = {}) {
  callGuestToReturn(core, {address, kScreen, arguments, std::nullopt, "memory screen"});
}

void screenOverride(Core *core) {
  session(*core).memoryScreen.enter(*core);
}

} // namespace

void MemoryScreen::install(Core &core) {
  phase_ = Phase::idle;
  psx::cpu::installNativeOverride(core, kScreen, "memory-screen", screenOverride);
}

void MemoryScreen::enter(Core &core) {
  if (!session(core).yieldAtFieldBarrier) {
    lucent::error("ts2-frame", "memory screen 0x{:08X} entered outside a field-spanning call", kScreen);
    std::abort();
  }
  if (phase_ == Phase::idle) {
    const std::uint32_t graphic = core.r[4];
    {
      const GuestRegisters registers(core);
      call(core, kScreenReset);
      call(core, kShowGraphic, {graphic});
    }
    remaining_ = kHoldFields;
    startFrom_ = core.mem_r16s(kColdStartFlag) != 0 ? kHoldFields : kFirstBootLock;
    result_ = 0;
    yieldField(core);
    return;
  }
  {
    const GuestRegisters registers(core);
    call(core, kAdvanceFrame);
    call(core, kDrawGraphic, {0, static_cast<std::uint32_t>(-8)});
    call(core, kSubmitFrame);
    const auto elapsed = static_cast<std::int32_t>(core.mem_r32(kElapsedFields));
    bool fading = remaining_ < kFadeFields;
    if (remaining_ > 0) {
      remaining_ -= elapsed;
      fading = remaining_ < kFadeFields;
      if (remaining_ < 1) {
        remaining_ = 0;
        fading = true;
      }
    }
    if (fading && remaining_ + elapsed >= kFadeFields) {
      call(core, kFadeOut, {0, 0, 0, 0xC});
    }
    // Retail takes Start only below startFrom_; the title skip takes it whenever the screen is not already fading.
    if ((core.mem_r16(kPadPressed) & kStart) != 0 && remaining_ >= kFadeFields) {
      remaining_ = kFadeFields;
      result_ = static_cast<std::uint32_t>(static_cast<std::int32_t>(core.mem_r16s(kColdStartFlag)));
    }
    const bool newDirection =
        (core.mem_r16(kPadPressed) & kDirections) != 0 && (core.mem_r16(kPadPrevious) & kDirections) == 0;
    if (newDirection && remaining_ < startFrom_ && remaining_ >= kFadeFields) {
      remaining_ = kFadeFields;
    }
  }
  if (remaining_ != 0) {
    yieldField(core);
    return;
  }
  phase_ = Phase::idle;
  core.r[2] = result_;
}

// The loop's barrier 0x8003FA68(1): publish the field, then end the host turn; the next turn re-enters here.
void MemoryScreen::yieldField(Core &core) {
  publishFieldBarrier(core, 1);
  session(core).displayedAtBarrier = true;
  phase_ = Phase::atBarrier;
  psx::cpu::requestExecutionExit(
      core, psx::cpu::ExecutionResult{psx::cpu::ExecutionExitReason::FrameBoundary, kScreen, 0, "memory screen field"});
}

} // namespace ts2
