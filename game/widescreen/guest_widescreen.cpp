#include "widescreen/guest_widescreen.h"

namespace ts2 {
namespace {

const psx::present::GuestWidescreenProjection kPolicy;

} // namespace

const psx::present::GuestWidescreenProjection &guestWidescreenPolicy() {
  return kPolicy;
}

} // namespace ts2
