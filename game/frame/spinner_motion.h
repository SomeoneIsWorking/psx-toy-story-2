// spinner_motion.h — the world objects the effect pass 0x80044C18 spins: in the draw it steps each one's instance
// angles by the fields elapsed and rebuilds its group matrix from them, so an in-between blends the angles it
// starts from.
#pragma once

#include "motion_set.h"

#include <cstdint>
#include <vector>

class Core;

namespace ts2 {

class SpinnerMotion final : public psx::present::MotionSource {
public:
  [[nodiscard]] const char *name() const override {
    return "spinners";
  }
  void sample(Core &core) override;
  psx::present::MotionCount write(psx::present::GuestRedraw &redraw, float t) const override;

private:
  // A pool record's instance and its angle words +0x0C and +0x10; its step depends on the record's index.
  struct Spinner {
    std::uint32_t record = 0;
    std::uint32_t instance = 0;
    std::uint32_t turnXY = 0;
    std::uint32_t turnZ = 0;
  };

  std::vector<Spinner> from_;
  std::vector<Spinner> to_;
};

} // namespace ts2
