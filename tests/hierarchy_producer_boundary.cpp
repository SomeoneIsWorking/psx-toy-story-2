// Hierarchy producer boundary: the ported model drawer 0x8002AC40's keys and its state render, through the shipping
// override.

#include "blend.h"
#include "facts/guest_facts.h"
#include "gte_control.h"
#include "render/hierarchy_drawer.h"
#include "render/ordering_tables.h"
#include "runtime/session.h"
#include "state_render_check.h"
#include "testutil.h"

#include <cstdint>
#include <span>
#include <utility>

namespace {

using namespace ts2::test;
using psx::present::GteControl;
using psx::present::RecordKey;

constexpr uint32_t kObject = 0x800C3100u; // the world object record the caller holds in s7
constexpr uint32_t kModel = 0x800B7000u;
constexpr uint32_t kMatrix = 0x800B7800u;
constexpr uint32_t kPosition = 0x800B7840u;
constexpr uint32_t kBuckets = 0x800B8000u;
constexpr uint32_t kOt = kTableBase + 0x10u;
constexpr uint32_t kClipRect = 0x1F800060u;
constexpr uint32_t kBias = 2u;
constexpr uint32_t kLiftScale = 0x1000u;
constexpr uint32_t kJointBytes = 12u;
constexpr uint32_t kQuadBytes = 0x2Cu;
constexpr uint32_t kPoolBefore = 0x801E0000u;
constexpr uint32_t kPoolAfter = 0x801E4000u;
constexpr uint32_t kPoolHalf = 0x801E8000u;

struct Joint {
  int32_t x;
  int32_t y;
  int32_t z;
  uint32_t quads;
};

// Two joints; the second's last quad sits behind the camera and is culled.
constexpr Joint kJoints[] = {{40, -30, 0, 2u}, {-60, 20, 30, 3u}};

void writeQuad(Core &core, uint32_t at, uint32_t index, bool behind) {
  // Four overlapping vertices over nine halfwords; the last is also the lift.
  const int16_t corners[] = {-40, -40, 0, 40, -40, 0, 40, 40, 16};
  for (uint32_t i = 0; i != 9u; ++i) {
    const int32_t value = behind && i % 3u == 2u ? -3000 : corners[i] + static_cast<int16_t>(index * 7u);
    core.mem_w16(at + i * 2u, static_cast<uint16_t>(value));
  }
  core.mem_w16(at + 0x12u, static_cast<uint16_t>((index % 3u) | (index == 1u ? 0x60u : 0x20u)));
  for (uint32_t uv = 0; uv != 4u; ++uv) {
    core.mem_w16(at + 0x14u + uv * 2u, static_cast<uint16_t>(0x4010u + uv * 0x0808u + index));
  }
  for (uint32_t colour = 0; colour != 4u; ++colour) {
    core.mem_w32(at + 0x1Cu + colour * 4u, 0x00406080u + colour * 0x101010u + index);
  }
}

void writeModel(Core &core) {
  core.mem_w32(kModel, 2u);
  uint32_t at = kModel + 4u;
  for (const Joint &joint : kJoints) {
    core.mem_w32(at, static_cast<uint32_t>(joint.x));
    core.mem_w32(at + 4u, static_cast<uint32_t>(joint.y));
    core.mem_w32(at + 8u, static_cast<uint32_t>(joint.z));
    at += kJointBytes;
  }
  uint32_t index = 0;
  for (const Joint &joint : kJoints) {
    core.mem_w32(at, joint.quads);
    at += 4u;
    for (uint32_t quad = 0; quad != joint.quads; ++quad) {
      writeQuad(core, at, index, index == 4u);
      at += kQuadBytes;
      ++index;
    }
  }
}

// The colour matrix the joints are turned by, near identity, and the model's position.
struct Pose {
  int32_t positionX;
  uint32_t matrixTurn; // R12 of the colour matrix
};

void writePose(Core &core, Pose pose) {
  const uint32_t matrix[] = {0x1000u | (pose.matrixTurn << 16), 0u, 0x1000u, 0u, 0x1000u};
  for (uint32_t word = 0; word != 5u; ++word) {
    core.mem_w32(kMatrix + word * 4u, matrix[word]);
  }
  core.mem_w32(kPosition, static_cast<uint32_t>(pose.positionX));
  core.mem_w32(kPosition + 4u, 10u);
  core.mem_w32(kPosition + 8u, 1200u);
}

void writeScene(Core &core) {
  ts2::HierarchyDrawer::install(core, ts2::session(core).stateProducers);
  ts2::OrderingTables::name(core);
  for (uint32_t otz = 0; otz < 0x240u; ++otz) {
    core.mem_w16(kBuckets + otz * 4u, static_cast<uint16_t>(((otz * 5u) & 0x3Fu) * 4u));
  }
  core.mem_w32(ts2::facts::kBucketOffsets, kBuckets);
  core.mem_w32(ts2::facts::kOrderingTablePointer, kOt);
  core.mem_w32(ts2::facts::kDepthLimit, 0x200u);
  const int16_t clip[] = {-200, 200, -150, 150}; // left, right, top, bottom
  for (uint32_t edge = 0; edge != 4u; ++edge) {
    core.mem_w16(kClipRect + 2u + edge * 4u, static_cast<uint16_t>(clip[edge]));
  }
  for (uint32_t page = 0; page != 3u; ++page) {
    const uint32_t entry = ts2::facts::kTexturePages + page * 16u;
    core.mem_w16(entry, static_cast<uint16_t>(0x0010u + page));
    core.mem_w16(entry + 2u, static_cast<uint16_t>(0x7800u + page * 0x40u));
    core.mem_w16(entry + 8u, page == 1u ? 1u : 0u);
  }
  writeModel(core);
  writePose(core, {.positionX = 0, .matrixTurn = 0u});
  const uint32_t control[][2] = {{0u, 0x1000u},
                                 {1u, 0u},
                                 {2u, 0x1000u},
                                 {3u, 0u},
                                 {4u, 0x1000u},
                                 {24u, 0u},
                                 {25u, 0u},
                                 {26u, 256u},
                                 {30u, 0x100u}};
  for (const auto &[reg, value] : control) {
    core.gte().writeControl(reg, value);
  }
}

bool draw(Core &core, uint32_t pool) {
  for (uint32_t bucket = 0; bucket != ts2::facts::kOrderingTableBuckets; ++bucket) {
    core.mem_w32(kTableBase + bucket * 4u, 0u);
  }
  core.mem_w32(ts2::facts::kPacketCursor, pool);
  core.r[4] = kModel;
  core.r[5] = kMatrix;
  core.r[6] = kPosition;
  core.r[7] = kLiftScale;
  core.r[ts2::HierarchyDrawer::kCallerObjectRegister] = kObject;
  core.mem_w32(0x801FFF10u, kBias);
  return call(core, ts2::HierarchyDrawer::kEntry);
}

std::span<const std::byte> savedState(const psx::present::FrameState &collected) {
  return required(collected.find({ts2::HierarchyDrawer::kEntry, kObject}), "the call saved a state");
}

// Everything the drawer reads of the guest, overwritten. The bucket offsets are level data and stay.
void scrambleInputs(Core &core) {
  for (const auto &[from, bytes] : {std::pair{kModel, 0x400u},
                                    std::pair{kMatrix, 0x20u},
                                    std::pair{kPosition, 0x10u},
                                    std::pair{ts2::facts::kTexturePages, ts2::facts::kTexturePageTableBytes},
                                    std::pair{0x1F800000u, 0x400u}}) {
    for (uint32_t at = from; at != from + bytes; at += 4u) {
      core.mem_w32(at, 0xA5A5A5A5u);
    }
  }
  for (const uint32_t global : {ts2::facts::kOrderingTablePointer,
                                ts2::facts::kPacketCursor,
                                ts2::facts::kBucketOffsets,
                                ts2::facts::kDepthLimit}) {
    core.mem_w32(global, 0xA5A5A5A5u);
  }
  for (uint32_t reg = 0; reg != 32u; ++reg) {
    if (reg != 31u) {
      core.gte().writeControl(reg, 0xA5A5A5A5u);
    }
  }
}

void test_quads_are_keyed_by_the_callers_object_and_model_quad() {
  auto game = residentGame();
  Core &core = game->core();
  writeScene(core);
  CHECK(draw(core, kPoolBefore));
  CHECK_EQ(core.r[2], 0u);
  // Quad 0 takes the first packet; the next drawn packet is quad 1's.
  CHECK(core.emission.identityFor(kPoolBefore) == (RecordKey{ts2::HierarchyDrawer::kEntry, kObject, 0u, 0u}));
  CHECK(core.emission.identityFor(kPoolBefore + 0x34u) == (RecordKey{ts2::HierarchyDrawer::kEntry, kObject, 1u, 0u}));
}

void test_render_at_one_reproduces_the_guest_packets_with_memory_scrambled() {
  auto game = residentGame();
  Core &core = game->core();
  writeScene(core);
  CHECK(draw(core, kPoolBefore));
  const auto expected = guestPrimitives(core);
  CHECK_EQ(expected.size(), 4u);
  const psx::present::FrameState state = collectedState(core, kPoolBefore);
  const std::span<const std::byte> saved = savedState(state);

  scrambleInputs(core);
  CHECK(samePrimitives(renderState(core, ts2::HierarchyDrawer::kEntry, saved, saved, 1.0f), expected));
}

// The camera turned about the view axis by about half a radian.
GteControl movedCamera(const GteControl &control) {
  GteControl moved = control;
  moved[0] = (static_cast<uint32_t>(static_cast<uint16_t>(-0x7AB)) << 16) | 0xE0Fu;
  moved[1] = (0x7ABu << 16) | (control[1] & 0xFFFFu);
  moved[2] = (control[2] & 0xFFFF0000u) | 0xE0Fu;
  return moved;
}

// Drawn with the camera, the colour matrix and the position moved, the render halfway equals the guest drawing
// with all three halfway.
void test_render_halfway_equals_a_frame_drawn_with_the_pose_halfway() {
  auto game = residentGame();
  Core &core = game->core();
  writeScene(core);
  const GteControl before = psx::present::readGteControl();
  const GteControl after = movedCamera(before);
  CHECK(draw(core, kPoolBefore));
  const psx::present::FrameState first = collectedState(core, kPoolBefore);
  psx::present::writeGteControl(after);
  writePose(core, {.positionX = 120, .matrixTurn = 0x0200u});
  CHECK(draw(core, kPoolAfter));
  const psx::present::FrameState second = collectedState(core, kPoolAfter);

  psx::present::writeGteControl(psx::present::blendGteControl(before, after, 0.5f));
  writePose(core,
            {.positionX = psx::present::lerpInt(0, 120, 0.5f),
             .matrixTurn = psx::present::lerpHalves(0x1000u, 0x02001000u, 0.5f) >> 16});
  CHECK(draw(core, kPoolHalf));
  const auto expected = guestPrimitives(core);
  const GteControl untouched = psx::present::readGteControl();

  const auto render = [&](float t) {
    return renderState(core, ts2::HierarchyDrawer::kEntry, savedState(first), savedState(second), t);
  };
  CHECK(samePrimitives(render(0.5f), expected));
  CHECK(psx::present::readGteControl() == untouched);
  CHECK(!samePrimitives(render(1.0f), expected));
  CHECK(!samePrimitives(render(0.0f), expected));
}

} // namespace

int main() {
  RUN(quads_are_keyed_by_the_callers_object_and_model_quad);
  RUN(render_at_one_reproduces_the_guest_packets_with_memory_scrambled);
  RUN(render_halfway_equals_a_frame_drawn_with_the_pose_halfway);
  return pt_summary();
}
