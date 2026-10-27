// debug_menu.h — Toy Story 2's debug menu: the level warp through the outer loop's level start.
#pragma once

#include "title_debug_menu.h"

namespace ts2 {

class DebugMenu final : public psx::game::TitleDebugMenu {
public:
  DebugMenu();

  // The level and shared-slot modules as the disc provisioner flattens them into scratch/flat.
  std::optional<psx::game::DecompImage>
  decompImage(Core &core, std::string_view imageName, bool resident) const override;
};

} // namespace ts2
