// Stock-libcd boundary: the runtime binds the synchronous CD bodies at the retail entries, the guest-RAM
// DMA callback table and the guest-interrupt ready delivery.

#include "cd/file_transfer.h"
#include "core.h"
#include "game.h"
#include "platform_hle.h"
#include "stock_dma_callbacks.h"
#include "stock_libcd.h"
#include "testutil.h"
#include "title_game.h"

namespace {

// Stock libcd entries in SLUS_008.93, and the bounds of the library they sit in.
constexpr uint32_t kCdGetSector = 0x80091108u;
constexpr uint32_t kCdSync = 0x80091898u;
constexpr uint32_t kCdCommand = 0x80091DE4u;
constexpr uint32_t kCdSearchFile = 0x80092AE8u;
constexpr uint32_t kCdRead = 0x80093AF0u;
constexpr uint32_t kCdReadSync = 0x80093BF4u;
constexpr uint32_t kCdLastPosition = 0x800A0820u;
constexpr uint32_t kCdReadyCallback = 0x800A0808u;
// The STR ring the FMV player waits on: its pop 0x800940F4, its post 0x80093E88 and the post's one call
// site 0x80094B30 are guest code inside the libcd span.
constexpr uint32_t kStrPop = 0x800940F4u;
constexpr uint32_t kStrPost = 0x80093E88u;
constexpr uint32_t kStrPostCall = 0x80094B30u;
constexpr uint32_t kDmaCallbackTable = 0x8009FD60u;

void test_measured_stock_libcd_entries_are_native_owned() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  const PlatformHle &table = title.game().platform_hle;

  for (const uint32_t entry : {kCdCommand, kCdSync, kCdGetSector, kCdRead, kCdReadSync, kCdSearchFile}) {
    CHECK(table.lookup(entry) != nullptr);
  }
  CHECK(table.lookup(kCdGetSector - 4) == nullptr);
  CHECK(title.game().cd.stockLibrary() != nullptr);
}

void test_sync_reports_completed_and_clears_result() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  Core &core = title.core();
  const uint32_t result = 0x80010000u;
  for (uint32_t i = 0; i < 8; ++i) {
    core.mem_w8(result + i, 0xA5u);
  }

  core.r[5] = result;
  title.game().platform_hle.lookup(kCdSync)(&core);

  CHECK_EQ(core.r[2], 2u);
  for (uint32_t i = 0; i < 8; ++i) {
    CHECK_EQ(core.mem_r8(result + i), 0u);
  }
}

void test_setloc_preserves_guest_bookkeeping_and_native_head_position() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  Core &core = title.core();
  const uint32_t position = 0x80010020u;
  const uint32_t result = 0x80010030u;
  const uint8_t requested[] = {0x00u, 0x02u, 0x10u, 0x00u};
  for (uint32_t i = 0; i < 4; ++i) {
    core.mem_w8(position + i, requested[i]);
  }
  for (uint32_t i = 0; i < 8; ++i) {
    core.mem_w8(result + i, 0xA5u);
  }

  core.r[4] = 0x02u; // CdlSetloc
  core.r[5] = position;
  core.r[6] = result;
  title.game().platform_hle.lookup(kCdCommand)(&core);

  CHECK_EQ(core.r[2], 0u);
  CHECK_EQ(title.game().cd.setloc_lba, 10);
  for (uint32_t i = 0; i < 4; ++i) {
    CHECK_EQ(core.mem_r8(kCdLastPosition + i), requested[i]);
  }
  for (uint32_t i = 0; i < 8; ++i) {
    CHECK_EQ(core.mem_r8(result + i), 0u);
  }
}

// Binding exact entries must leave the STR ring's pop and post running as guest code.
void test_str_ring_code_stays_guest_code() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  for (const uint32_t entry : {kStrPop, kStrPost, kStrPostCall}) {
    CHECK(title.game().platform_hle.lookup(entry) == nullptr);
  }
}

// libapi keeps the per-channel callbacks in guest RAM, where the IRQ3 dispatcher 0x800890C4 reads them.
void test_dma_callbacks_come_from_the_guest_table() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  Core &core = title.core();
  const psx::hle::StockDmaCallbacks *table = title.game().dmaCallbacks.guestTable();
  CHECK(table != nullptr);
  core.mem_w32(kDmaCallbackTable + 3u * 4u, 0x80091234u);
  CHECK_EQ(table->callback(core, 3), 0x80091234u);
  CHECK_EQ(table->callback(core, 2), 0u);
}

// FMV data-ready delivery: the guest's INT1 service calls CdReadyCallback, so the framework leaves it there.
void test_cd_ready_delivery_belongs_to_the_guest_interrupt() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  const psx::cd::StockLibcd *libcd = title.game().cd.stockLibrary();
  CHECK(libcd != nullptr);
  CHECK_EQ(libcd->readySlot(), kCdReadyCallback);
  CHECK(libcd->deliversOnInterrupt());
}

// The native whole-file read refuses an unreadable path, a non-guest-RAM destination and a bad file name.
// A refusal transfers nothing, reports the guest's failure value and says why. All cases are refused before
// the disc is consulted.
void test_file_transfer_refuses_rather_than_transferring() {
  ts2::test::TitleGame title;
  Core &core = title.core();
  const ts2::cd::FileTransfer transfer;

  const uint32_t sentinel = 0x80100000u;
  const uint32_t pathSlot = 0x80101000u;
  const uint32_t barePathSlot = 0x80101100u;
  const uint32_t longPathSlot = 0x80101200u;
  constexpr uint8_t kSentinel = 0x5Au;
  for (uint32_t i = 0; i < 32u; ++i) {
    core.mem_w8(sentinel + i, kSentinel);
  }
  const char *path = "SLUS_008.93;1";
  for (uint32_t i = 0; i < 13u; ++i) {
    core.mem_w8(pathSlot + i, static_cast<uint8_t>(path[i]));
  }
  core.mem_w8(barePathSlot, 0u);
  // A name with no terminator inside the owner's bound is not a disc name, whatever the disc holds.
  for (uint32_t i = 0; i < 256u; ++i) {
    core.mem_w8(longPathSlot + i, static_cast<uint8_t>('A' + (i % 26u)));
  }

  const auto outsideRam = transfer.transfer(core, {.path = 0x00000000u, .destination = sentinel});
  CHECK(!outsideRam.transferred);
  CHECK_EQ(outsideRam.bytes, 0u);
  CHECK(!outsideRam.why.empty());

  const auto pastRam = transfer.transfer(core, {.path = pathSlot, .destination = 0x80200000u});
  CHECK(!pastRam.transferred);
  CHECK_EQ(pastRam.bytes, 0u);
  CHECK(!pastRam.why.empty());

  const auto emptyName = transfer.transfer(core, {.path = barePathSlot, .destination = sentinel});
  CHECK(!emptyName.transferred);
  CHECK_EQ(emptyName.bytes, 0u);
  CHECK(!emptyName.why.empty());

  const auto unterminated = transfer.transfer(core, {.path = longPathSlot, .destination = sentinel});
  CHECK(!unterminated.transferred);
  CHECK_EQ(unterminated.bytes, 0u);
  CHECK(!unterminated.why.empty());

  // Nothing was written: the sentinel is intact and the guest's transfer words are untouched.
  for (uint32_t i = 0; i < 32u; ++i) {
    CHECK_EQ(core.mem_r8(sentinel + i), kSentinel);
  }
  core.mem_w32(0x800A1034u, 0x1234u);
  core.mem_w32(0x800A1588u, 0x5678u);
  (void)transfer.transfer(core, {.path = longPathSlot, .destination = sentinel});
  CHECK_EQ(core.mem_r32(0x800A1034u), 0x1234u);
  CHECK_EQ(core.mem_r32(0x800A1588u), 0x5678u);
}

} // namespace

int main() {
  RUN(measured_stock_libcd_entries_are_native_owned);
  RUN(sync_reports_completed_and_clears_result);
  RUN(setloc_preserves_guest_bookkeeping_and_native_head_position);
  RUN(str_ring_code_stays_guest_code);
  RUN(dma_callbacks_come_from_the_guest_table);
  RUN(cd_ready_delivery_belongs_to_the_guest_interrupt);
  RUN(file_transfer_refuses_rather_than_transferring);
  return pt_summary();
}
