#include "frame/draw_motion.h"

#include "blend.h"
#include "core.h"
#include "gte_control.h"
#include "render/actor_incarnation.h"
#include "runtime/session.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ts2 {
namespace {

using psx::present::Motion;
using psx::present::MotionField;
using psx::present::MotionKey;
using psx::present::MotionRecords;
using psx::present::MotionTrack;

// 0x8002C848 leaves the camera's MATRIX at 0x1F800394 (RotMatrix of its angles, the x row widened, the rows scaled by
// the shake gp+0x9C4), its position at 0x1F800374 and that position >> 5 at 0x1F800384; the published angles
// 0x800A1618 are three s16.
constexpr std::uint32_t kCameraMatrix = 0x1F800394u;
constexpr auto kCameraMatrixFields = psx::present::transformFields(0);
constexpr std::uint32_t kCameraPosition = 0x1F800374u;
constexpr std::array kCameraPositionFields{
    MotionField{.offset = 0x00},
    MotionField{.offset = 0x04},
    MotionField{.offset = 0x08},
    MotionField{.offset = 0x10},
    MotionField{.offset = 0x14},
    MotionField{.offset = 0x18},
};
constexpr std::uint32_t kCameraAngles = 0x800A1618u;
constexpr std::array kCameraAngleFields{MotionField{.offset = 0x00, .motion = Motion::Angle12x2},
                                        MotionField{.offset = 0x04, .motion = Motion::Angle12Low}};

// gp+0x898 (s16): the sprite frame counter 0x80044C18 steps by the fields elapsed; its sprites show frame >> 1.
constexpr std::uint32_t kSpriteFrame = 0x800A1570u;
constexpr std::array kSpriteFrameFields{MotionField{.offset = 0x00}};

// ActorMotion's words, in its order.
constexpr std::array kActorFields{
    MotionField{.offset = 0x00},
    MotionField{.offset = 0x04},
    MotionField{.offset = 0x08},
    MotionField{.offset = 0x0C},
    MotionField{.offset = 0x10},
    MotionField{.offset = 0x14},
    MotionField{.offset = 0x18},
    MotionField{.offset = 0x1C},
};
constexpr std::size_t kTurnXY = 3;
constexpr std::size_t kTurnZ = 4;
constexpr std::size_t kModel = 5;
using PoseLayer = ActorMotion::PoseLayer;
constexpr std::array kPoseLayers{PoseLayer{.frame = 6, .animation = kTurnZ},
                                 PoseLayer{.frame = 7, .animation = kModel}};
// The model index +0x14: another model is another object.
constexpr std::array kActorKeys{MotionKey{.offset = 0x14, .mask = 0xFFFFu}};

constexpr std::uint32_t kPlayer = 0x800B2188u;
// 64 records of 0x98 bytes; slot i is bound to placement 0x800B57A8 + i * 0x20.
constexpr std::uint32_t kActorPool = 0x800C2EA0u;
constexpr std::uint32_t kActorBytes = 0x98u;
constexpr std::uint32_t kActorCount = 64u;
// The active actors, a zero-ended pointer list 0x8007863C copies into the draw list.
constexpr std::uint32_t kActiveList = 0x800B18A8u;

// The model table: a model's animation table starts at +8, one pointer each; an animation's +4 holds its frame
// count in the low 15 bits.
constexpr std::uint32_t kModels = 0x800C7268u;
constexpr std::uint32_t kAnimations = 8u;
constexpr std::uint32_t kFrameCountOffset = 4u;
constexpr std::uint32_t kFrameCountMask = 0x7FFFu;
constexpr std::uint32_t kNoAnimation = 0xFFFFu;
constexpr unsigned kFrameFractionBits = 16u;

// The part table: a model's parts run from the index its +4 holds to the one its +6 holds.
constexpr std::uint32_t kParts = 0x800A9618u;
constexpr std::uint32_t kPartBytes = 0x6Cu;
constexpr std::uint32_t kPartCount = 160u;
constexpr std::uint32_t kModelFirstPart = 4u;
constexpr std::uint32_t kModelPartsEnd = 6u;
constexpr std::array kPartFields{
    MotionField{.offset = 0x00},
    MotionField{.offset = 0x04},
    MotionField{.offset = 0x08},
    MotionField{.offset = 0x58},
    MotionField{.offset = 0x5C},
    MotionField{.offset = 0x60},
    MotionField{.offset = 0x64},
    MotionField{.offset = 0x68},
};
// The draw list 0x8007863C builds before the section, zero-ended; the player is its last entry.
constexpr std::uint32_t kDrawList = 0x800A4298u;
constexpr std::uint32_t kDrawListMax = kActorCount + 1u;

std::uint32_t half(std::uint32_t word, unsigned shift) {
  return (word >> shift) & 0xFFFFu;
}

} // namespace

std::uint32_t ActorMotion::frameCount(std::span<const std::uint32_t> words, const PoseLayer &layer) const {
  const std::uint32_t model = half(words[kModel], 0);
  const std::uint32_t animation = half(words[layer.animation], 16);
  const std::uint32_t entry = core_.mem_r32(kModels + model * 4u);
  if (entry == 0u) {
    return 0u;
  }
  const std::uint32_t header = core_.mem_r32(entry + kAnimations + animation * 4u);
  return header == 0u ? 0u : core_.mem_r16(header + kFrameCountOffset) & kFrameCountMask;
}

void ActorMotion::blend(const Samples &samples, float t, std::span<std::uint32_t> out) const {
  const std::span<const std::uint32_t> from = samples.from;
  const std::span<const std::uint32_t> to = samples.to;
  for (std::size_t word = 0; word < kTurnXY; ++word) {
    out[word] = psx::present::blendedWord(Motion::Linear, from[word], to[word], t);
  }
  out[kTurnXY] = psx::present::blendedWord(Motion::Angle12x2, from[kTurnXY], to[kTurnXY], t);
  out[kTurnZ] = psx::present::blendedWord(Motion::Angle12Low, from[kTurnZ], to[kTurnZ], t);
  out[kModel] = to[kModel];
  for (const PoseLayer &layer : kPoseLayers) {
    const std::size_t frame = layer.frame;
    out[frame] = to[frame];
    const std::uint32_t animation = half(to[layer.animation], 16);
    if (animation == kNoAnimation || animation != half(from[layer.animation], 16)) {
      continue;
    }
    const auto period = static_cast<std::int64_t>(frameCount(to, layer)) << kFrameFractionBits;
    if (period == 0 || from[frame] >= period || to[frame] >= period) {
      continue;
    }
    const double at = psx::present::lerpRing(static_cast<std::int32_t>(from[frame]),
                                             static_cast<std::int32_t>(to[frame]),
                                             static_cast<std::int32_t>(period),
                                             t);
    out[frame] = static_cast<std::uint32_t>(static_cast<std::int64_t>(std::nearbyint(at)) % period);
  }
}

void PartMotion::blend(const Samples &samples, float t, std::span<std::uint32_t> out) const {
  for (std::size_t word = 0; word < 3u; ++word) {
    out[word] = psx::present::blendedWord(Motion::Linear, samples.from[word], samples.to[word], t);
  }
  psx::gte::RotationWords from{};
  psx::gte::RotationWords to{};
  std::copy_n(samples.from.begin() + 3, from.size(), from.begin());
  std::copy_n(samples.to.begin() + 3, to.size(), to.begin());
  const psx::gte::RotationWords rotation = psx::present::rotationBetween(from, to, t);
  std::copy(rotation.begin(), rotation.end(), out.begin() + 3);
  out[7] = (samples.to[7] & 0xFFFF0000u) | (rotation[4] & 0xFFFFu);
}

void PartLiveness::sample(Core &core) {
  std::swap(earlier_, posed_);
  posed_.clear();
  std::unordered_map<std::uint32_t, std::uint32_t> drawn;
  std::unordered_map<std::uint32_t, std::uint32_t> drawnBy;
  std::unordered_set<std::uint32_t> unanimated;
  for (std::uint32_t entry = 0; entry < kDrawListMax; ++entry) {
    const std::uint32_t actor = core.mem_r32(kDrawList + entry * 4u);
    if (actor == 0u) {
      break;
    }
    const std::uint32_t model = core.mem_r16(actor + 0x14u);
    ++drawn[model];
    drawnBy[model] = actor;
    if (core.mem_r16(actor + 0x12u) == kNoAnimation && core.mem_r16(actor + 0x16u) == kNoAnimation) {
      unanimated.insert(model);
    }
  }
  const ActorIncarnations &incarnations = session(core).actorIncarnations;
  for (const auto &[model, draws] : drawn) {
    const std::uint32_t entry = core.mem_r32(kModels + model * 4u);
    if (draws != 1u || !unanimated.contains(model) || entry == 0u) {
      continue;
    }
    const std::uint32_t actor = drawnBy[model];
    const Owner owner{.actor = actor, .generation = incarnations.generation(actor)};
    const auto first = static_cast<std::uint32_t>(core.mem_r16s(entry + kModelFirstPart));
    const auto end = static_cast<std::uint32_t>(core.mem_r16s(entry + kModelPartsEnd));
    for (std::uint32_t part = first; part < end && part < kPartCount; ++part) {
      posed_.emplace(kParts + part * kPartBytes, owner);
    }
  }
}

bool PartLiveness::live(std::uint32_t record) const {
  const auto now = posed_.find(record);
  if (now == posed_.end()) {
    return false;
  }
  const auto before = earlier_.find(record);
  return before == earlier_.end() || before->second == now->second;
}

void ActorLiveness::sample(Core &core) {
  listed_.clear();
  reborn_.clear();
  for (std::uint32_t entry = kActiveList;; entry += 4u) {
    const std::uint32_t actor = core.mem_r32(entry);
    if (actor == 0u || listed_.size() == kActorCount) {
      break;
    }
    listed_.insert(actor);
  }
  const ActorIncarnations &incarnations = session(core).actorIncarnations;
  for (std::uint32_t slot = 0; slot < kActorCount; ++slot) {
    const std::uint32_t actor = kActorPool + slot * kActorBytes;
    const std::uint32_t generation = incarnations.generation(actor);
    const auto [seen, inserted] = generations_.try_emplace(actor, generation);
    if (!inserted && seen->second != generation) {
      reborn_.insert(actor);
      seen->second = generation;
    }
  }
}

bool ActorLiveness::live(std::uint32_t record) const {
  return listed_.contains(record) && !reborn_.contains(record);
}

DrawMotion::DrawMotion(Core &core)
    : actor_(core),
      tracks_{{
          {.name = "camera matrix", .address = kCameraMatrix, .fields = kCameraMatrixFields, .rule = &camera_},
          {.name = "camera position", .address = kCameraPosition, .fields = kCameraPositionFields},
          {.name = "camera angles", .address = kCameraAngles, .fields = kCameraAngleFields},
          {.name = "player", .address = kPlayer, .fields = kActorFields, .rule = &actor_, .keys = kActorKeys},
          {.name = "actors",
           .address = kActorPool,
           .fields = kActorFields,
           .rule = &actor_,
           .keys = kActorKeys,
           .records = MotionRecords{.stride = kActorBytes, .count = kActorCount, .liveness = &actors_}},
          {.name = "posed parts",
           .address = kParts,
           .fields = kPartFields,
           .rule = &part_,
           .records = MotionRecords{.stride = kPartBytes, .count = kPartCount, .liveness = &parts_}},
          {.name = "sprite frame", .address = kSpriteFrame, .fields = kSpriteFrameFields, .rule = &spriteFrame_},
      }} {}

} // namespace ts2
