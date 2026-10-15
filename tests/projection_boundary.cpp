// Projection boundary: the runtime binds the stock libgte, libgpu and VSync bodies at the title's addresses,
// and they mutate CPU/GTE state like retail. Hermetic.

#include "core.h"
#include "game.h"
#include "platform_hle.h"
#include "testutil.h"
#include "title_game.h"

namespace {

constexpr uint32_t kSetGeomOffset = 0x80083CD4u;
constexpr uint32_t kSetGeomScreen = 0x80083CF4u;
constexpr uint32_t kGpuTimeoutArm = 0x80088380u;
constexpr uint32_t kGpuTimeoutCheck = 0x800883B4u;
constexpr uint32_t kGpuTimeoutDeadline = 0x8009EC20u;
constexpr uint32_t kGpuTimeoutFlag = 0x8009EC24u;
constexpr uint32_t kVSync = 0x80088628u;

void test_registration_binds_projection_gpu_timeout_and_vsync_at_their_entries() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  const PlatformHle &table = title.game().platform_hle;

  CHECK(table.lookup(kSetGeomOffset) != nullptr);
  CHECK(table.lookup(kSetGeomScreen) != nullptr);
  CHECK(table.lookup(kGpuTimeoutArm) != nullptr);
  CHECK(table.lookup(kGpuTimeoutCheck) != nullptr);
  CHECK(table.lookup(kVSync) != nullptr);
  CHECK(table.lookup(kSetGeomOffset + 4) == nullptr);
  CHECK(table.lookup(kVSync + 4) == nullptr);
  CHECK(table.lookup(0x8003A650u) == nullptr); // title graphics init is never a stock body
}

void test_gpu_timeout_pair_completes_without_guest_vsync() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  Core &core = title.core();
  OverrideFn arm = title.game().platform_hle.lookup(kGpuTimeoutArm);
  OverrideFn check = title.game().platform_hle.lookup(kGpuTimeoutCheck);
  CHECK(arm != nullptr);
  CHECK(check != nullptr);

  core.mem_w32(kGpuTimeoutDeadline, 0);
  core.mem_w32(kGpuTimeoutFlag, 9);
  arm(&core);
  CHECK_EQ(core.mem_r32(kGpuTimeoutDeadline), 0x7FFFFFFFu);
  CHECK_EQ(core.mem_r32(kGpuTimeoutFlag), 0u);
  core.r[2] = 0xFFFFFFFFu;
  check(&core);
  CHECK_EQ(core.r[2], 0u);
}

void test_offset_leaf_preserves_retail_state_and_records_projection() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  Core &core = title.core();
  OverrideFn setOffset = title.game().platform_hle.lookup(kSetGeomOffset);
  CHECK(setOffset != nullptr);

  core.r[4] = 256u;
  core.r[5] = 120u;
  setOffset(&core);

  CHECK_EQ(core.r[4], 256u << 16);
  CHECK_EQ(core.r[5], 120u << 16);
  CHECK_EQ(core.gte().readControl(24), 256u << 16);
  CHECK_EQ(core.gte().readControl(25), 120u << 16);
  CHECK_EQ(core.rsub.projParams.geomOfx(), 256.0f);
  CHECK_EQ(core.rsub.projParams.geomOfy(), 120.0f);
  CHECK(!core.rsub.projParams.geomValid());
}

void test_screen_leaf_completes_same_core_projection() {
  ts2::test::TitleGame title;
  title.registerOverrides();
  Core &core = title.core();
  OverrideFn setOffset = title.game().platform_hle.lookup(kSetGeomOffset);
  OverrideFn setScreen = title.game().platform_hle.lookup(kSetGeomScreen);
  CHECK(setOffset != nullptr);
  CHECK(setScreen != nullptr);

  core.r[4] = 256u;
  core.r[5] = 120u;
  setOffset(&core);
  core.r[4] = 160u;
  setScreen(&core);

  CHECK_EQ(core.r[4], 160u); // retail SetGeomScreen does not mutate a0
  CHECK_EQ(core.gte().readControl(26), 160u);
  CHECK_EQ(core.rsub.projParams.geomH(), 160.0f);
  CHECK(core.rsub.projParams.geomValid());
}

} // namespace

int main() {
  RUN(registration_binds_projection_gpu_timeout_and_vsync_at_their_entries);
  RUN(gpu_timeout_pair_completes_without_guest_vsync);
  RUN(offset_leaf_preserves_retail_state_and_records_projection);
  RUN(screen_leaf_completes_same_core_projection);
  return psx::test::summary();
}
