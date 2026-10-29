#pragma once

#include <cstdint>

class Core;

namespace ts2 {

// Guest screen columns the picture shows, left inclusive and right exclusive.
struct ScreenColumns {
  std::int32_t left = 0;
  std::int32_t right = 0;
};

// The guest keeps its retail 512x240 canvas and projection centre; only the two rectangles it culls against
// its console frame are widened, so objects in the record canvas's margins are still submitted.
class ResidentWidescreenCull {
public:
  static void install(Core &core);

  // Whether the record present latched a picture wider than the console's 4:3.
  static bool active(const Core &core);

  // The retail columns 0..511 plus the record canvas's margins on each side.
  static ScreenColumns drawnColumns(const Core &core);

  // Widens the rectangle handed to the cull leaf at 0x80027AF0 to the signed range its boxes are stored in.
  static void widenCullRect(Core &core);

  // Widens the rectangle the mesh submitter publishes at 0x80010000.
  static void widenScreenRect(Core &core);
};

} // namespace ts2
