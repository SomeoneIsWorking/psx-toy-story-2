// SLUS_008.93 graphics-buffer facts shared by the boot, frame and render owners.
#pragma once

#include <cstdint>

namespace ts2::facts {

// Graphics init 0x80039D9C builds the buffer objects at 0x801BBD28/0x801DD21C. Each holds its ClearOTagR
// table (0x80085F5C, 0x57C buckets) at +0x2B4; the packets the frame's drawers link into it come from the
// pool at +0x1924.
inline constexpr uint32_t kGraphicsBufferA = 0x801BBD28u;
inline constexpr uint32_t kGraphicsBufferB = 0x801DD21Cu;
inline constexpr uint32_t kOrderingTableOffset = 0x2B4u;
inline constexpr uint32_t kOrderingTableBuckets = 0x57Cu;
inline constexpr uint32_t kPacketPoolOffset = 0x1924u;
inline constexpr uint32_t kPacketPoolStride = kGraphicsBufferB - kGraphicsBufferA;

// The frame's drawers take packets at this cursor (gp+0x930) and add bucket offsets to the ordering table
// pointer (gp+0x3E4, bucket 4 of the selected buffer's table).
inline constexpr uint32_t kPacketCursor = 0x800A1608u;
inline constexpr uint32_t kOrderingTablePointer = 0x800A10BCu;
// Pointer to the u16 bucket offsets, one per OTZ at OTZ * 4.
inline constexpr uint32_t kBucketOffsets = 0x800A12F8u;
// gp+0x684: the deepest OTZ a primitive may take.
inline constexpr uint32_t kDepthLimit = 0x800A135Cu;
// Texture page entries, 16 bytes each: tpage, then CLUT base.
inline constexpr uint32_t kTexturePages = 0x800CD200u;
inline constexpr uint32_t kTexturePageBytes = 16u;
inline constexpr uint32_t kTexturePageTableBytes = 0x200u;

// VSync(-1) returns this word (0x8008866C lw 0x800A0000-0x2AC); the frame driver mirrors the host field count.
inline constexpr uint32_t kVSyncQueryCounter = 0x8009FD54u;

} // namespace ts2::facts
