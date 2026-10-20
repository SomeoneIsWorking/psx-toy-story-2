#pragma once

#include "guest_widescreen_projection.h"

class Core;

namespace ts2 {

// Whether the picture is wider than 4:3.
const psx::present::GuestWidescreenProjection &guestWidescreenPolicy();

} // namespace ts2
