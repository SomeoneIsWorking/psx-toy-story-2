#include "render/mesh_submitters.h"

#include "core.h"
#include "emit_memory.h"
#include "execution_exit.h"
#include "native_dispatch.h"
#include "render/mesh_drawers.h"

#include <lucent/log.h>

#include <cstdlib>
#include <optional>

namespace ts2 {
namespace {

constexpr std::uint32_t kScratch = 0x1F800000u;

// s0..s6 are spilled to the scratchpad before the body.
void spillSavedRegisters(Core &core) {
  for (std::uint32_t s = 0; s != 7u; ++s) {
    core.mem_w32(kScratch + s * 4u, core.r[16u + s]);
  }
}

template <std::uint32_t Submitter> void submit(Core *core) {
  const MeshDrawCall call{
      Submitter, {core->r[4], core->r[5], core->r[6], core->r[7]}, core->r[3], core->r[31], core->r[22]};
  const psx::present::EmitMemory guest(*core);
  if (!MeshDrawers::plan(guest, call)) {
    psx::cpu::callOriginalToReturn(*core, Submitter, psx::cpu::ExecutionBudget::currentTurn(*core), "mesh submitter");
    return;
  }
  spillSavedRegisters(*core);
  const std::optional<MeshDrawResult> result = MeshDrawers::run(guest, call);
  if (!result) {
    lucent::error("ts2-mesh", "planned call 0x{:08X} on mesh 0x{:08X} left its native body", Submitter, call.args[0]);
    std::abort();
  }
  core->r[2] = result->v0;
  core->r[3] = result->v1;
}

} // namespace

void MeshSubmitters::install(Core &core) {
  psx::cpu::installNativeOverride(core, kStaticMeshSubmitter, "static-mesh-submitter", &submit<kStaticMeshSubmitter>);
  psx::cpu::installNativeOverride(core, kRigidMeshDrawer, "rigid-mesh-drawer", &submit<kRigidMeshDrawer>);
}

} // namespace ts2
