#include "render/hierarchy_draw_state.h"

#include "blend.h"
#include "core.h"
#include "emit_memory.h"
#include "facts/guest_facts.h"
#include "gte_control.h"
#include "host_ordering_table.h"
#include "render/ordering_tables.h"
#include "state_bytes.h"

#include <cstring>
#include <optional>
#include <span>

namespace ts2 {
namespace {

using psx::present::EmitMemory;
using psx::present::GteControl;
using psx::present::HostMemory;

constexpr std::uint32_t kScratch = 0x1F800000u;
constexpr std::uint32_t kScratchBytes = 0x400u;
constexpr std::uint32_t kMatrixWords = 5u; // the rotation the colour matrix is loaded from, as s16 pairs
constexpr std::uint32_t kPositionWords = 3u;

struct StateHeader {
  HierarchyDrawCall call;
  std::uint32_t tableBase = 0; // bucket 0 of the ordering table the drawer links into
  std::uint32_t cursor = 0;    // the packet cursor at entry
  std::uint32_t arenaBytes = 0;
  std::uint32_t ranges = 0;
  GteControl control{};
};
static_assert(std::is_trivially_copyable_v<StateHeader>);

struct HierarchyState {
  StateHeader header;
  std::vector<MemoryRange> ranges;
  std::vector<std::byte> bytes;

  static HierarchyState read(std::span<const std::byte> state) {
    psx::present::StateReader reader(state);
    HierarchyState saved;
    saved.header = reader.get<StateHeader>();
    saved.ranges = reader.getAll<MemoryRange>(saved.header.ranges);
    std::size_t total = 0;
    for (const MemoryRange &range : saved.ranges) {
      total += range.size;
    }
    saved.bytes = reader.getAll<std::byte>(total);
    return saved;
  }

  // The bytes of input range `index`.
  std::span<std::byte> range(std::uint32_t index) {
    std::size_t at = 0;
    for (std::uint32_t i = 0; i != index; ++i) {
      at += ranges[i].size;
    }
    return std::span(bytes).subspan(at, ranges[index].size);
  }

  // The same model drawn.
  bool sameDraw(const HierarchyState &other) const {
    const MemoryRange &model = ranges[HierarchyDrawer::kModelRange];
    const MemoryRange &otherModel = other.ranges[HierarchyDrawer::kModelRange];
    return model.address == otherModel.address && model.size == otherModel.size;
  }
};

std::uint32_t wordAt(std::span<const std::byte> bytes, std::uint32_t word) {
  std::uint32_t value = 0;
  std::memcpy(&value, bytes.data() + std::size_t{word} * 4u, sizeof(value));
  return value;
}

void setWord(std::span<std::byte> bytes, std::uint32_t word, std::uint32_t value) {
  std::memcpy(bytes.data() + std::size_t{word} * 4u, &value, sizeof(value));
}

// `to`'s inputs with the rotation, the matrix and the position moved `t` of the way from `from`.
void blendInto(HierarchyState &to, HierarchyState &from, float t) {
  to.header.control = psx::present::blendGteControl(from.header.control, to.header.control, t);
  const std::span<std::byte> matrix = to.range(HierarchyDrawer::kMatrixRange);
  const std::span<const std::byte> fromMatrix = from.range(HierarchyDrawer::kMatrixRange);
  for (std::uint32_t word = 0; word != kMatrixWords; ++word) {
    setWord(matrix, word, psx::present::lerpHalves(wordAt(fromMatrix, word), wordAt(matrix, word), t));
  }
  const std::span<std::byte> position = to.range(HierarchyDrawer::kPositionRange);
  const std::span<const std::byte> fromPosition = from.range(HierarchyDrawer::kPositionRange);
  for (std::uint32_t word = 0; word != kPositionWords; ++word) {
    setWord(position,
            word,
            static_cast<std::uint32_t>(psx::present::lerpInt(static_cast<std::int32_t>(wordAt(fromPosition, word)),
                                                             static_cast<std::int32_t>(wordAt(position, word)),
                                                             t)));
  }
}

} // namespace

HierarchyDrawRecorder::HierarchyDrawRecorder(Core &core, const HierarchyDrawCall &call) {
  const EmitMemory guest(core);
  const std::uint32_t table = guest.mem_r32(facts::kOrderingTablePointer);
  const std::optional<psx::present::OtSlot> slot = core.otTables.slotOf(table);
  named_ = slot.has_value();
  if (!named_) {
    return;
  }
  const HierarchyDrawInputs inputs = HierarchyDrawer::inputs(guest, call);
  cursor_ = guest.mem_r32(facts::kPacketCursor);
  StateHeader header;
  header.call = call;
  header.tableBase = table - slot->index * sizeof(std::uint32_t);
  header.cursor = cursor_;
  header.arenaBytes = inputs.packetBytes;
  header.ranges = static_cast<std::uint32_t>(inputs.ranges.size());
  header.control = psx::present::readGteControl(core.gte());
  psx::present::StateWriter writer;
  writer.put(header);
  writer.putAll(std::span<const MemoryRange>(inputs.ranges));
  std::vector<std::byte> bytes;
  for (const MemoryRange &range : inputs.ranges) {
    for (std::uint32_t offset = 0; offset != range.size; ++offset) {
      bytes.push_back(static_cast<std::byte>(guest.mem_r8(range.address + offset)));
    }
  }
  writer.putAll(std::span<const std::byte>(bytes));
  const std::span<const std::byte> written = writer.bytes();
  state_.assign(written.begin(), written.end());
}

void HierarchyDrawRecorder::save(Core &core) const {
  if (!named_ || core.mem_r32(facts::kPacketCursor) == cursor_) {
    return;
  }
  core.frameStates.save(core.emission.current(), std::span<const std::byte>(state_));
}

void HierarchyStateRender::render(const psx::present::StateInterval &states, psx::present::PrimitiveSink &sink) const {
  HierarchyState state = HierarchyState::read(states.to);
  if (states.t < 1.0f && states.from.data() != states.to.data()) {
    HierarchyState earlier = HierarchyState::read(states.from);
    if (earlier.sameDraw(state)) {
      blendInto(state, earlier, states.t);
    }
  }

  HostMemory host;
  host.zero(kScratch, kScratchBytes);
  host.zero(state.header.tableBase, facts::kOrderingTableBuckets * sizeof(std::uint32_t));
  host.zero(state.header.cursor, state.header.arenaBytes);
  for (std::uint32_t index = 0; index != state.ranges.size(); ++index) {
    const MemoryRange &range = state.ranges[index];
    const std::span<const std::byte> bytes = state.range(index);
    if (range.address - kScratch < kScratchBytes) {
      host.write(range.address, bytes.data(), range.size);
    } else {
      host.provide(range.address, bytes);
    }
  }

  {
    const psx::present::GteGuard guard(core_.gte());
    psx::present::writeGteControl(core_.gte(), state.header.control);
    HierarchyDrawer::run(EmitMemory(core_, host), state.header.call);
  }
  psx::present::emitHostOrderingTable(host,
                                      psx::present::HostOrderingTable{state.header.tableBase,
                                                                      facts::kOrderingTableBuckets,
                                                                      psx::present::OtSlot{OrderingTables::kTableId, 0},
                                                                      state.header.arenaBytes / 4u},
                                      sink);
}

} // namespace ts2
