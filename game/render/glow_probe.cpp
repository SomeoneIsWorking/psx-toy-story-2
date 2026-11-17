#include "render/glow_probe.h"

#include "core.h"
#include "execution/guest_execution.h"
#include "facts/guest_facts.h"
#include "frame/draw_replay.h"
#include "gpu_device.h"
#include "native_dispatch.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace ts2 {
namespace {

constexpr std::uint32_t kGlowProbe = 0x80040160u;
// Draws the probe pixels and arms the strip flag of the buffer it drew for.
constexpr std::uint32_t kDrawProbes = 0x80040400u;
// Draws light `$a0`'s glow.
constexpr std::uint32_t kDrawGlow = 0x80040DB4u;
// libgpu StoreImage / LoadImage (RECT *, u_long *).
constexpr std::uint32_t kStoreImage = 0x80085D7Cu;
constexpr std::uint32_t kLoadImage = 0x80085D18u;
// gp+0x3CC: the buffer index the probe and the glows use.
constexpr std::uint32_t kProbeBuffer = 0x800A10A4u;
// s16 per buffer: the strip holds probe pixels to read back.
constexpr std::uint32_t kStripArmed = 0x800A1568u;
// s32 per buffer, cleared with the flag.
constexpr std::uint32_t kStripCount = 0x800A12E0u;
// The strip: 8x1 at (0x3C0, 0x7F - 2 * buffer).
constexpr std::uint16_t kStripX = 0x3C0u;
constexpr std::uint16_t kStripBottom = 0x7Fu;
constexpr std::uint32_t kStripPixels = 8u;
constexpr std::uint16_t kLightReached = 0x3E0u;
constexpr std::uint16_t kLightReachedDim = 0x3C0u;
constexpr std::uint16_t kStripCleared = 0x7FFu;
constexpr std::uint32_t kVramWidth = 1024u;
// The guest's frame: RECT at sp+0x10, the pixels at sp+0x18.
constexpr std::uint32_t kFrameBytes = 0x50u;
constexpr std::uint32_t kRectOffset = 0x10u;
constexpr std::uint32_t kPixelsOffset = 0x18u;

void probeOverride(Core *core) {
  GlowProbe::run(*core);
}

bool call(Core &core, std::uint32_t address, std::uint32_t a0 = 0, std::uint32_t a1 = 0) {
  core.r[4] = a0;
  core.r[5] = a1;
  return psx::cpu::dispatchGuestOrPropagate(core, address, "glow probe");
}

} // namespace

void GlowProbe::install(Core &core) {
  psx::cpu::installNativeOverride(core, kGlowProbe, "glow-probe", probeOverride);
}

void GlowProbe::run(Core &core) {
  const GuestRegisters saved(core);
  core.r[29] -= kFrameBytes;
  const std::uint32_t frame = core.r[29];
  if (!call(core, kDrawProbes)) {
    return;
  }
  const std::uint32_t buffer = core.mem_r32(facts::kDrawBuffer) == facts::kGraphicsBufferA ? 0u : 1u;
  core.mem_w32(kProbeBuffer, buffer);
  if (core.mem_r16(kStripArmed + buffer * 2u) != 0u) {
    const std::uint32_t rect = frame + kRectOffset;
    const std::uint32_t pixels = frame + kPixelsOffset;
    const auto stripY = static_cast<std::uint16_t>(kStripBottom - buffer * 2u);
    core.mem_w16(rect + 0u, kStripX);
    core.mem_w16(rect + 2u, stripY);
    core.mem_w16(rect + 4u, kStripPixels);
    core.mem_w16(rect + 6u, 1u);
    const bool replay = replaying(core);
    if (replay) {
      // Nothing draws between the replay and the real run's read of the same strip.
      const std::span<const std::uint16_t> vram = core.gpuDevice().vram();
      for (std::uint32_t i = 0; i < kStripPixels; ++i) {
        core.mem_w16(pixels + i * 2u, vram[static_cast<std::size_t>(stripY) * kVramWidth + kStripX + i]);
      }
    } else if (!call(core, kStoreImage, rect, pixels)) {
      return;
    }
    for (std::uint32_t i = 0; i < kStripPixels; ++i) {
      const std::uint16_t pixel = core.mem_r16(pixels + i * 2u);
      if ((pixel == kLightReached || pixel == kLightReachedDim) && !call(core, kDrawGlow, i)) {
        return;
      }
      core.mem_w16(pixels + i * 2u, kStripCleared);
    }
    if (!replay && !call(core, kLoadImage, rect, pixels)) {
      return;
    }
  }
  core.mem_w16(kStripArmed + buffer * 2u, 0u);
  core.mem_w32(kStripCount + buffer * 4u, 0u);
}

} // namespace ts2
