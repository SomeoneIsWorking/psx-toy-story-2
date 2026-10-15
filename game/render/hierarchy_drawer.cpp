#include "render/hierarchy_drawer.h"

#include "core.h"
#include "emission_scope.h"
#include "facts/guest_facts.h"
#include "gte_registers.h"
#include "native_dispatch.h"
#include "render/hierarchy_draw_state.h"

#include <array>
#include <cstdint>
#include <memory>

namespace ts2 {

using psx::present::EmitMemory;

namespace {

constexpr std::uint32_t kClipRect = 0x1F800060u; // s16 left, right, top, bottom at +2, +6, +0xA, +0xE
constexpr std::uint32_t kClipRectBytes = 0x10u;
constexpr std::uint32_t kScreen = 0x1F800200u; // the four projected corners RotAverage4 leaves
constexpr std::uint32_t kJointBytes = 12u;     // one joint offset, three words
constexpr std::uint32_t kQuadBytes = 0x2Cu;
constexpr std::uint32_t kPacketBytes = 0x34u;
constexpr std::uint32_t kMvmvaColourMatrix = 0x4A4DA012u; // MVMVA sf=1 LCM * IR + BK
constexpr std::uint32_t kTranslation = 0x14u;             // the matrix's translation words

struct ClipRect {
  std::int32_t left;
  std::int32_t right;
  std::int32_t top;
  std::int32_t bottom;
};

// The model: a joint count, the joints, then per joint a quad count and its quads.
struct Quad {
  std::uint32_t at;
  std::uint32_t index; // across the whole model, the quad's element
};

std::int32_t screenX(const EmitMemory &m, std::uint32_t corner) {
  return m.mem_r16s(kScreen + corner * 4u);
}

std::int32_t screenY(const EmitMemory &m, std::uint32_t corner) {
  return m.mem_r16s(kScreen + corner * 4u + 2u);
}

// True when every corner fails `outside`.
template <class Outside> bool allOutside(Outside outside) {
  for (std::uint32_t corner = 0; corner != 4u; ++corner) {
    if (!outside(corner)) {
      return false;
    }
  }
  return true;
}

// 0x80084024 (RotAverage4) over a quad's four overlapping vertex pairs; returns OTZ and the flag after RTPT.
struct Average {
  std::uint32_t otz;
  std::uint32_t firstFlag;
};

Average rotAverage4(const EmitMemory &m, psx::cpu::Gte &gte, std::uint32_t quad) {
  gte.writeData(psx::gte::kVxy0, m.mem_r32(quad));
  gte.writeData(psx::gte::kVz0, m.mem_r32(quad + 4u));
  gte.writeData(psx::gte::kVxy1, m.mem_r32(quad + 4u));
  gte.writeData(psx::gte::kVz1, m.mem_r32(quad + 8u));
  gte.writeData(psx::gte::kVxy2, m.mem_r32(quad + 8u));
  gte.writeData(psx::gte::kVz2, m.mem_r32(quad + 0xCu));
  gte.execute(psx::gte::kRtpt);
  m.mem_w32(kScreen, gte.readData(psx::gte::kSxy0));
  m.mem_w32(kScreen + 4u, gte.readData(psx::gte::kSxy1));
  m.mem_w32(kScreen + 8u, gte.readData(psx::gte::kSxy2));
  const std::uint32_t flag = gte.readControl(psx::gte::kFlag);
  gte.writeData(psx::gte::kVxy0, m.mem_r32(quad + 0xCu));
  gte.writeData(psx::gte::kVz0, m.mem_r32(quad + 0x10u));
  gte.execute(psx::gte::kRtps);
  m.mem_w32(kScreen + 0xCu, gte.readData(psx::gte::kSxy2));
  gte.execute(psx::gte::kAvsz4);
  return {gte.readData(psx::gte::kOtz), flag};
}

// A guest bucket offset rounded toward zero to a word.
std::uint32_t bucketWord(const EmitMemory &m, std::uint32_t entry) {
  std::int32_t offset = m.mem_r16s(entry);
  if (offset < 0) {
    offset += 3;
  }
  return static_cast<std::uint32_t>(offset >> 2) << 2;
}

// A drawn quad's OTZ and the call's depth bias, which together pick its bucket.
struct Depth {
  std::uint32_t otz;
  std::uint32_t bias;
};

// The GT4 packet: corners from the scratchpad, colours and UVs from the quad, its page and CLUT from the page table.
void link(const EmitMemory &m, Quad drawn, Depth depth) {
  const std::uint32_t quad = drawn.at;
  const std::uint32_t p = m.mem_r32(facts::kPacketCursor);
  m.mem_w32(p + 0x08u, m.mem_r32(kScreen));
  m.mem_w32(p + 0x14u, m.mem_r32(kScreen + 4u));
  m.mem_w32(p + 0x2Cu, m.mem_r32(kScreen + 8u));
  m.mem_w32(p + 0x20u, m.mem_r32(kScreen + 0xCu));
  m.mem_w32(p + 0x04u, m.mem_r32(quad + 0x1Cu));
  m.mem_w32(p + 0x10u, m.mem_r32(quad + 0x20u));
  m.mem_w32(p + 0x28u, m.mem_r32(quad + 0x24u));
  m.mem_w32(p + 0x1Cu, m.mem_r32(quad + 0x28u));
  m.mem_w16(p + 0x0Cu, m.mem_r16(quad + 0x14u));
  m.mem_w16(p + 0x18u, m.mem_r16(quad + 0x16u));
  m.mem_w16(p + 0x30u, m.mem_r16(quad + 0x18u));
  m.mem_w8(p + 3u, 0x0Cu);
  m.mem_w16(p + 0x24u, m.mem_r16(quad + 0x1Au));
  m.mem_w8(p + 7u, 0x3Cu);
  const std::uint32_t page = m.mem_r16(quad + 0x12u);
  const std::uint32_t entry = facts::kTexturePages + ((page & 0x1Fu) << 4);
  m.mem_w16(p + 0x1Au, static_cast<std::uint16_t>(m.mem_r16(entry) + (page & 0x60u)));
  if (m.mem_r16s(entry + 8u) == 0) {
    const std::uint32_t u0 = m.mem_r8(p + 0x0Cu);
    const std::uint32_t v0 = m.mem_r8(p + 0x0Du);
    m.mem_w16(p + 0x0Eu, static_cast<std::uint16_t>(((v0 & 0xC0u) >> 4) + m.mem_r16(entry + 2u) + (u0 >> 6)));
  } else {
    m.mem_w16(p + 0x0Eu, m.mem_r16(entry + 2u));
  }
  if ((page & 0x60u) != 0x60u) {
    m.mem_w8(p + 7u, static_cast<std::uint8_t>(m.mem_r8(p + 7u) | 2u));
  }
  const std::uint32_t bucket = ((depth.otz + depth.bias) << 2) + m.mem_r32(facts::kBucketOffsets);
  const std::uint32_t table = m.mem_r32(facts::kOrderingTablePointer);
  const std::uint32_t head = bucketWord(m, bucket) + table;
  m.mem_w32(p, (m.mem_r32(p) & 0xFF000000u) | (m.mem_r32(head) & 0x00FFFFFFu));
  const std::uint32_t slot = bucketWord(m, bucket) + table;
  m.mem_w32(facts::kPacketCursor, p + kPacketBytes);
  m.mem_w32(slot, (m.mem_r32(slot) & 0xFF000000u) | (p & 0x00FFFFFFu));
}

class Walk {
public:
  Walk(const EmitMemory &m, const HierarchyDrawCall &call) : m_(m), gte_(m.core().gte()), call_(call) {}

  HierarchyDrawResult run() {
    HierarchyDrawResult result{1u, call_.v1};
    const std::uint32_t model = call_.args[0];
    const std::uint32_t matrix = call_.args[1];
    const std::uint32_t position = call_.args[2];
    clip_ = {m_.mem_r16s(kClipRect + 2u),
             m_.mem_r16s(kClipRect + 6u),
             m_.mem_r16s(kClipRect + 0xAu),
             m_.mem_r16s(kClipRect + 0xEu)};
    const auto joints = static_cast<std::int32_t>(m_.mem_r32(model));
    for (std::uint32_t i = 0; i != psx::gte::kLightColourWords; ++i) {
      gte_.writeControl(psx::gte::kLightColour + i, m_.mem_r32(matrix + i * 4u));
    }
    for (std::uint32_t i = 0; i != 3u; ++i) {
      gte_.writeControl(psx::gte::kBackRed + i, m_.mem_r32(position + i * 4u));
    }
    std::uint32_t joint = model + 4u;
    std::uint32_t quads = joint + static_cast<std::uint32_t>(joints) * kJointBytes;
    std::uint32_t index = 0;
    for (std::int32_t left = joints; left > 0; --left) {
      const std::array<std::uint32_t, 3> offset = turn(joint);
      const auto count = static_cast<std::int32_t>(m_.mem_r32(quads));
      std::uint32_t quad = quads + 4u;
      for (std::int32_t remaining = count; remaining > 0; --remaining) {
        const psx::present::ElementScope element(m_, index);
        draw(Quad{quad, index}, offset, result);
        quad += kQuadBytes;
        ++index;
      }
      quads = quad;
      joint += kJointBytes;
    }
    for (std::uint32_t i = 0; i != 3u; ++i) {
      gte_.writeControl(psx::gte::kBackRed + i, 0u);
    }
    return result;
  }

private:
  // 0x8001F2C0: the joint's offset through the colour matrix, plus the model's position.
  std::array<std::uint32_t, 3> turn(std::uint32_t joint) {
    gte_.writeData(psx::gte::kIr1, m_.mem_r32(joint));
    gte_.writeData(psx::gte::kIr2, m_.mem_r32(joint + 4u));
    gte_.writeData(psx::gte::kIr3, m_.mem_r32(joint + 8u));
    gte_.execute(kMvmvaColourMatrix);
    return {gte_.readData(psx::gte::kMac1), gte_.readData(psx::gte::kMac2), gte_.readData(psx::gte::kMac3)};
  }

  void draw(Quad quad, const std::array<std::uint32_t, 3> &offset, HierarchyDrawResult &result) {
    const std::uint32_t matrix = call_.args[1];
    const std::uint32_t bias = call_.args[4];
    for (std::uint32_t i = 0; i != 3u; ++i) {
      m_.mem_w32(matrix + kTranslation + i * 4u, offset[i]);
    }
    const std::int64_t product = std::int64_t{m_.mem_r16s(quad.at + 0x10u)} * static_cast<std::int32_t>(call_.args[3]);
    auto lift = static_cast<std::int32_t>(static_cast<std::uint32_t>(product));
    if (lift < 0) {
      lift += 0xFFF;
    }
    const std::uint32_t lifted = m_.mem_r32(matrix + kTranslation + 8u) + static_cast<std::uint32_t>(lift >> 12);
    m_.mem_w32(matrix + kTranslation + 8u, lifted);
    for (std::uint32_t i = 0; i != 3u; ++i) {
      gte_.writeControl(psx::gte::kTranslationX + i, m_.mem_r32(matrix + kTranslation + i * 4u));
    }
    const Average average = rotAverage4(m_, gte_, quad.at);
    result.v1 = average.firstFlag;
    if (static_cast<std::int32_t>(average.otz) <= 0) {
      return;
    }
    result.v1 = bias << 2;
    if (static_cast<std::int32_t>(average.otz) >=
        static_cast<std::int32_t>(m_.mem_r32(facts::kDepthLimit) + (bias << 2))) {
      return;
    }
    if (outside(result)) {
      return;
    }
    link(m_, quad, Depth{average.otz, bias});
    result.v0 = 0;
    result.v1 = (m_.mem_r32(facts::kPacketCursor) - kPacketBytes) & 0x00FFFFFFu;
  }

  // The four tests in the guest's order; v1 holds the bound of the last one made.
  bool outside(HierarchyDrawResult &result) const {
    result.v1 = static_cast<std::uint32_t>(clip_.right);
    if (allOutside([&](std::uint32_t c) {
          return clip_.right < screenX(m_, c);
        })) {
      return true;
    }
    result.v1 = static_cast<std::uint32_t>(clip_.left);
    if (allOutside([&](std::uint32_t c) {
          return screenX(m_, c) < clip_.left;
        })) {
      return true;
    }
    result.v1 = static_cast<std::uint32_t>(clip_.bottom);
    if (allOutside([&](std::uint32_t c) {
          return clip_.bottom < screenY(m_, c);
        })) {
      return true;
    }
    result.v1 = static_cast<std::uint32_t>(clip_.top);
    return allOutside([&](std::uint32_t c) {
      return screenY(m_, c) < clip_.top;
    });
  }

  const EmitMemory &m_;
  psx::cpu::Gte &gte_;
  const HierarchyDrawCall &call_;
  ClipRect clip_{};
};

void overrideDrawer(Core *core) {
  const HierarchyDrawCall call{{core->r[4], core->r[5], core->r[6], core->r[7], core->mem_r32(core->r[29] + 0x10u)},
                               core->r[3]};
  const EmitMemory guest(*core);
  const psx::present::EmissionScope::Guard object(
      core->emission, HierarchyDrawer::kEntry, core->r[HierarchyDrawer::kCallerObjectRegister], 0);
  const HierarchyDrawRecorder recorder(*core, call);
  const HierarchyDrawResult result = HierarchyDrawer::run(guest, call);
  recorder.save(*core);
  core->r[2] = result.v0;
  core->r[3] = result.v1;
}

} // namespace

HierarchyDrawResult HierarchyDrawer::run(const EmitMemory &memory, const HierarchyDrawCall &call) {
  return Walk(memory, call).run();
}

HierarchyDrawInputs HierarchyDrawer::inputs(const EmitMemory &m, const HierarchyDrawCall &call) {
  const std::uint32_t model = call.args[0];
  const auto joints = static_cast<std::int32_t>(m.mem_r32(model));
  std::uint32_t end = model + 4u + static_cast<std::uint32_t>(joints > 0 ? joints : 0) * kJointBytes;
  std::uint32_t quads = 0;
  for (std::int32_t joint = 0; joint < joints; ++joint) {
    const auto count = static_cast<std::int32_t>(m.mem_r32(end));
    const std::uint32_t drawn = count > 0 ? static_cast<std::uint32_t>(count) : 0u;
    end += 4u + drawn * kQuadBytes;
    quads += drawn;
  }
  HierarchyDrawInputs inputs;
  inputs.ranges.push_back({model, end - model});
  inputs.ranges.push_back({call.args[1], kMatrixBytes});
  inputs.ranges.push_back({call.args[2], kPositionBytes});
  inputs.ranges.push_back({kClipRect, kClipRectBytes});
  inputs.ranges.push_back({facts::kTexturePages, facts::kTexturePageTableBytes});
  for (const std::uint32_t global :
       {facts::kDepthLimit, facts::kPacketCursor, facts::kBucketOffsets, facts::kOrderingTablePointer}) {
    inputs.ranges.push_back({global, sizeof(std::uint32_t)});
  }
  inputs.packetBytes = quads * kPacketBytes;
  return inputs;
}

void HierarchyDrawer::install(Core &core, psx::present::StateProducerTable &renders) {
  psx::cpu::installNativeOverride(core, kEntry, "hierarchy-drawer", &overrideDrawer);
  renders.install(kEntry, std::make_unique<HierarchyStateRender>(core));
}

} // namespace ts2
