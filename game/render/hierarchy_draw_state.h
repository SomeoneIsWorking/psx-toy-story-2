// A hierarchical model drawer call as a producer state, and its render.
//
// The state is what the drawer reads: its arguments, the GTE control registers the caller left (the rotation),
// and the guest memory `HierarchyDrawer::inputs` names. The render runs the drawer's body again over host copies
// with the rotation, the matrix the colour matrix is loaded from and the model's position `t` of the way between
// two states, and reads the packets it linked out of a host copy of the ordering table.
#pragma once

#include "render/hierarchy_drawer.h"
#include "state_producer.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class Core;

namespace ts2 {

// Reads a call's inputs before the body runs and saves them under the object scope open now, once the body has
// linked a packet. Nothing is saved when the drawer's table is not a named ordering table.
class HierarchyDrawRecorder {
public:
  HierarchyDrawRecorder(Core &core, const HierarchyDrawCall &call);

  void save(Core &core) const;

private:
  std::vector<std::byte> state_;
  std::uint32_t cursor_ = 0;
  bool named_ = false;
};

class HierarchyStateRender final : public psx::present::StateProducer {
public:
  explicit HierarchyStateRender(Core &core) : core_(core) {}

  void render(const psx::present::StateInterval &states, psx::present::PrimitiveSink &sink) const override;

private:
  Core &core_;
};

} // namespace ts2
