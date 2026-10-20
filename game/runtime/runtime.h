#pragma once

#include "game_runtime.h"
#include "input/recording_phase.h"
#include "stock_dma_callbacks.h"
#include "stock_libcd.h"
#include "stock_libgpu.h"
#include "stock_libgte.h"
#include "stock_pad_buffers.h"
#include "stock_vsync.h"

#include <cstdint>
#include <cstdio>

namespace ts2 {

// Replays crt0 and guest main's prefix, then FrameDriver owns each field. Records interpolate
// through the slot-mesh and actor-part producers.
class Runtime final : public psx::game::GameRuntime {
public:
  Runtime();

  void startProgram(Core &core) override;
  void registerOverrides(Game &game) override;
  std::unique_ptr<psx::game::TitleSessionState> createSession(Game &game) override;
  std::unique_ptr<psx::game::FrameDriver> createFrameDriver(Game &game) override;
  void deliverPad(Core &core, const psx::input::PadDelivery &delivery) override;
  std::optional<psx::gpu::PacketPoolExtent> packetPool(Core &core) const override;
  bool interpolates() const override {
    return true;
  }
  const psx::present::StateProducer *stateProducer(Core &core, std::uint32_t producer) const override;
  bool sealedFrameIsCut(Core &core) const override;
  const psx::present::GuestWidescreenProjection *guestWidescreenProjection() const override;
  // The title screen discards Start inside its 30-field lockout, so absolute pad frames would miss it.
  std::uint64_t inputPhase(Core &core) const override {
    return inputPhase_.of(core);
  }
  const psx::game::HostIdentity *hostIdentity() const override;
  bool titleCommand(Core &core, const psx::game::TitleCommandLine &input, std::FILE *reply) override;
  psx::game::TitleDevWarp *devWarp(Core &core) const override;

private:
  psx::hle::StockLibgte libgte_;
  psx::hle::StockVSync vsync_;
  psx::hle::StockLibgpu libgpu_;
  psx::hle::StockDmaCallbacks dmaCallbacks_;
  psx::cd::GuestInterruptLibcd libcd_;
  psx::input::StockPadBuffers pad_;
  InputPhase inputPhase_;
};

} // namespace ts2
