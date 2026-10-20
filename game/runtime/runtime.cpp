#include "runtime/runtime.h"

#include "cd/file_transfer.h"
#include "core.h"
#include "crt0_boot.h"
#include "facts/guest_facts.h"
#include "frame/frame_driver.h"
#include "game.h"
#include "guest_packet_pool_windows.h"
#include "input/pad_owner.h"
#include "overlay/overlay_images.h"
#include "render/actor_producers.h"
#include "render/hierarchy_drawer.h"
#include "render/ordering_tables.h"
#include "render/slot_mesh_producers.h"
#include "runtime/session.h"
#include "widescreen/guest_widescreen.h"
#include "widescreen/resident_widescreen.h"

#include <cstdlib>
#include <lucent/log.h>
#include <string_view>

namespace ts2 {
namespace {

constexpr psx::game::Crt0Image kCrt0{.entry = 0x80082D60u};

// Each parity's window runs from OT bucket 4 (+0x2C4, published at gp+0x3E4) to the next buffer, spanning the
// +0x1924 packet pool.
constexpr psx::gpu::GuestPacketPoolWindows kPacketPool{
    .representation = psx::gpu::GuestPacketPoolWindows::Representation::FixedBaseStride,
    .base = facts::kGraphicsBufferA + 0x2C4u,
    .stride = facts::kPacketPoolStride,
};

constexpr psx::game::HostIdentity kHostIdentity{
    .cardEnvVar = "PSXPORT_TS2_CARD",
    .cardDefaultPath = "scratch/saves/toystory2.mcr",
};

// Stock libcd from 0x80091108 to 0x80093C50. CdReadyCallback lives at 0x800A0808 and the guest's INT1
// service delivers it with libcd's data-ready code; a synchronous CdRead is polled through CdReadSync.
constexpr psx::cd::StockLibcd::Entries kLibcd{
    .read = 0x80093AF0u,
    .readSync = 0x80093BF4u,
    .command = 0x80091DE4u,
    .sync = 0x80091898u,
    .searchFile = 0x80092AE8u,
    .getSector = 0x80091108u,
};
constexpr psx::cd::StockCommandWorkArea kLibcdWorkArea{
    .lastPositionAddress = 0x800A0820u,
    .lastModeAddress = 0x800A0824u,
};
constexpr std::uint32_t kCdReadyCallback = 0x800A0808u;

// libgpu's DMA timeout pair derives its deadline from VSync(-1).
constexpr psx::hle::StockLibgpu::Entries kLibgpu{
    .timeoutArm = 0x80088380u,
    .timeoutCheck = 0x800883B4u,
    .timeoutDeadlineWord = 0x8009EC20u,
    .timeoutFlagWord = 0x8009EC24u,
};

// The frame driver owns the display fields, so guest VSync 0x80088628 is never entered as guest code.
constexpr std::uint32_t kVSync = 0x80088628u;
// libapi's per-channel DMA callback table, read by the IRQ3 dispatcher 0x800890C4 (lw at 0x80089154).
constexpr std::uint32_t kDmaCallbackTable = 0x8009FD60u;

} // namespace

Runtime::Runtime()
    : libgte_({.setGeomOffset = 0x80083CD4u, .setGeomScreen = 0x80083CF4u}),
      vsync_({.entry = kVSync, .queryCounter = facts::kVSyncQueryCounter}), libgpu_(kLibgpu),
      dmaCallbacks_(kDmaCallbackTable), libcd_(kLibcd, kLibcdWorkArea, kCdReadyCallback, {}),
      pad_({.slot0Buffer = PadOwner::kSlot0Buffer, .slot1Buffer = PadOwner::kSlot1Buffer}) {}

void Runtime::startProgram(Core &core) {
  psx::game::Crt0Boot{kCrt0}.run(core);
  session(core).guestMainBoot.initialize(core);
}

void Runtime::registerOverrides(Game &game) {
  libgte_.bind(game);
  vsync_.bind(game);
  libgpu_.bind(game);
  dmaCallbacks_.bind(game);
  libcd_.bind(game);
  Session &title = session(game.core);
  title.graphicsSync.install(game.core);
  title.overlays.installLoadObserver(game.core);
  cd::FileTransfer::install(game.core);
  title.soundBank.install(game.core);
  title.pad.install(game.core);
  OrderingTables::name(game.core);
  SlotMeshProducers::install(game.core, title.stateProducers);
  ActorIncarnations::install(game.core);
  ActorProducers::install(game.core, title.stateProducers);
  HierarchyDrawer::install(game.core, title.stateProducers);
  FrameCut::install(game.core);
  ResidentWidescreenCull::install(game.core);
}

std::unique_ptr<psx::game::TitleSessionState> Runtime::createSession(Game &) {
  return std::make_unique<Session>();
}

std::unique_ptr<psx::game::FrameDriver> Runtime::createFrameDriver(Game &game) {
  return ts2::createFrameDriver(game);
}

void Runtime::deliverPad(Core &core, const psx::input::PadDelivery &delivery) {
  pad_.deliver(core, delivery);
}

std::optional<psx::gpu::PacketPoolExtent> Runtime::packetPool(Core &core) const {
  return kPacketPool.extent(core);
}

const psx::present::StateProducer *Runtime::stateProducer(Core &core, std::uint32_t producer) const {
  return session(core).stateProducers.find(producer);
}

bool Runtime::sealedFrameIsCut(Core &core) const {
  return session(core).frameCut.isCut();
}

const psx::present::GuestWidescreenProjection *Runtime::guestWidescreenProjection() const {
  return &guestWidescreenPolicy();
}

const psx::game::HostIdentity *Runtime::hostIdentity() const {
  return &kHostIdentity;
}

bool Runtime::titleCommand(Core &core, const psx::game::TitleCommandLine &input, std::FILE *reply) {
  if (std::string_view(input.command) != "warp") {
    return false;
  }
  std::fprintf(reply, "%s\n", core.game->frameDriverAs<FrameDriver>().devWarp().arm(input.args).c_str());
  return true;
}

psx::game::TitleDevWarp *Runtime::devWarp(Core &core) const {
  auto *driver = core.game->frameDriverIf<FrameDriver>();
  return driver != nullptr ? &driver->devWarp() : nullptr;
}

} // namespace ts2
