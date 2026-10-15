#include "render/slot_mesh_producers.h"

#include "core.h"
#include "execution_exit.h"
#include "native_dispatch.h"
#include "render/mesh_draw_state.h"
#include "render/mesh_drawers.h"
#include "render/mesh_format.h"

#include <lucent/log.h>

#include <cstdlib>
#include <memory>
#include <optional>

namespace ts2 {
namespace {

constexpr std::uint32_t kScratch = 0x1F800000u;

std::uint32_t entriesPerPrimitive(std::uint32_t submitter, std::uint8_t opcode) {
  const bool doublePacket = submitter == SlotMeshProducers::kStaticMeshSubmitter && opcode >= 8u && opcode < 16u;
  return doublePacket ? 2u : 1u;
}

// The instance's slot table and packet base as they stood when the call began.
struct SlotBinding {
  std::uint32_t table = 0;
  std::uint32_t packets = 0;
};

template <std::uint32_t Submitter> void keyFaces(Core &core, std::uint32_t mesh, SlotBinding slots) {
  const std::uint32_t entries = SlotMeshProducers::slotEntries<Submitter>(core, mesh);
  for (std::uint32_t entry = 0; entry != entries; ++entry) {
    const std::uint32_t slot = core.mem_r16(slots.table + entry * 2u);
    if (slot == 0u) {
      continue;
    }
    const psx::present::EmissionScope::Guard face(core.emission, Submitter, slots.table, entry);
    core.emission.bindPacket(slots.packets + slot * 4u);
  }
}

// s0..s6 are spilled to the scratchpad before the body.
void spillSavedRegisters(Core &core) {
  for (std::uint32_t s = 0; s != 7u; ++s) {
    core.mem_w32(kScratch + s * 4u, core.r[16u + s]);
  }
}

// A call whose commands have no native body: the guest original, its faces keyed.
template <std::uint32_t Submitter> void submitOnGuest(Core *core) {
  const std::uint32_t mesh = core->r[4];
  const SlotBinding slots{core->mem_r32(kInstanceSlotTable), core->mem_r32(kSlotPacketBase)};
  psx::cpu::callOriginalToReturn(*core, Submitter, psx::cpu::ExecutionBudget::currentTurn(*core), "slot mesh producer");
  keyFaces<Submitter>(*core, mesh, slots);
}

template <std::uint32_t Submitter> void submit(Core *core) {
  const MeshDrawCall call{
      Submitter, {core->r[4], core->r[5], core->r[6], core->r[7]}, core->r[3], core->r[31], core->r[22]};
  const psx::present::EmitMemory guest(*core);
  const std::optional<MeshDrawInputs> inputs = MeshDrawers::plan(guest, call);
  if (!inputs) {
    submitOnGuest<Submitter>(core);
    return;
  }
  spillSavedRegisters(*core);
  // One object per call: the instance's slot table, which every face entry of the call indexes.
  const psx::present::EmissionScope::Guard object(core->emission, Submitter, guest.mem_r32(kInstanceSlotTable), 0);
  const MeshDrawRecorder recorder(*core, call, *inputs);
  const std::optional<MeshDrawResult> result = MeshDrawers::run(guest, call);
  if (!result) {
    lucent::error("ts2-mesh", "planned call 0x{:08X} on mesh 0x{:08X} left its native body", Submitter, call.args[0]);
    std::abort();
  }
  recorder.save(*core, *result);
  core->r[2] = result->v0;
  core->r[3] = result->v1;
}

} // namespace

// No dispatcher scope: it would key the body's unnamed pool packets by mesh, which pairs them by order.
void SlotMeshProducers::install(Core &core, psx::present::StateProducerTable &renders) {
  psx::cpu::installNativeOverride(core, kStaticMeshSubmitter, "static-mesh-producer", &submit<kStaticMeshSubmitter>);
  psx::cpu::installNativeOverride(core, kRigidMeshDrawer, "rigid-mesh-producer", &submit<kRigidMeshDrawer>);
  renders.install(kStaticMeshSubmitter, std::make_unique<MeshStateRender>(core));
  renders.install(kRigidMeshDrawer, std::make_unique<MeshStateRender>(core));
}

template <std::uint32_t Submitter> std::uint32_t SlotMeshProducers::slotEntries(Core &core, std::uint32_t mesh) {
  const std::optional<ResidentMeshLayout> layout = decodeResidentMeshLayout(psx::present::EmitMemory(core), mesh);
  // 0x80017FF8 reads only the positive vertex-count header.
  if (!layout || (Submitter == kRigidMeshDrawer && layout->hasAuxiliaryVertexRecords)) {
    return 0u;
  }
  std::uint32_t entries = 0;
  std::uint32_t address = layout->commandAddress;
  while (true) {
    const std::optional<ResidentMeshCommand> command =
        decodeResidentMeshCommand(psx::present::EmitMemory(core), address);
    if (!command) {
      return 0u;
    }
    if (command->terminal) {
      return entries;
    }
    entries += command->primitiveCount * entriesPerPrimitive(Submitter, command->opcode);
    address = command->nextCommandAddress;
  }
}

template std::uint32_t SlotMeshProducers::slotEntries<SlotMeshProducers::kStaticMeshSubmitter>(Core &, std::uint32_t);
template std::uint32_t SlotMeshProducers::slotEntries<SlotMeshProducers::kRigidMeshDrawer>(Core &, std::uint32_t);

} // namespace ts2
