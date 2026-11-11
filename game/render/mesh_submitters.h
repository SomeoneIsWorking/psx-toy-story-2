// The resident mesh submitters run on their native bodies, whose split paths keep pieces in the widescreen margins.
#pragma once

#include <cstdint>

class Core;

namespace ts2 {

class MeshSubmitters {
public:
  inline static constexpr std::uint32_t kStaticMeshSubmitter = 0x800100E4u;
  inline static constexpr std::uint32_t kRigidMeshDrawer = 0x80017FF8u;

  static void install(Core &core);
};

} // namespace ts2
