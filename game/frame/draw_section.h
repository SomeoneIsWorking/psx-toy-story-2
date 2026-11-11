// draw_section.h — the tail of the resident update (0x8007B254, or the orbit 0x8007B850) that builds the frame's
// OT, then the links the deferred display 0x8003A888 adds before its DrawOTag, as a replay section.
#pragma once

#include "replay_frame.h"

#include <cstdint>
#include <optional>

namespace ts2 {

class DrawSection final : public psx::present::ReplaySection {
public:
  // The section opens with facts::kDrawModePacket; 0x8002A070 is the world pass.
  inline static constexpr std::uint32_t kWorldPass = 0x8002A070u;

  // Whether the frame's update is the orbit 0x8007B850 rather than 0x8007B254.
  void select(bool orbit) {
    orbit_ = orbit;
  }

  // The steps from the draw-mode packet through the display's tint; the head of the buffer's OT.
  [[nodiscard]] std::optional<std::uint32_t> draw(Core &core, psx::present::GuestRedraw &redraw) const override;
  // The OT head DrawOTag 0x80086054 submits, of either buffer; PutDrawEnv's packet is the frame's other chain.
  [[nodiscard]] bool linked(std::uint32_t head) const override;

private:
  bool orbit_ = false;
};

} // namespace ts2
