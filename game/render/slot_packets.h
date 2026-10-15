// The retained packet a mesh drawer keeps per face entry: its texture page words, its first fill and its link
// into the ordering table. The rigid drawer and the static submitter write the same packets.
#pragma once

#include "emit_memory.h"

#include <array>
#include <cstdint>

namespace ts2 {

struct PacketShape {
  bool quad;
  bool textured;
};

struct SlotPair {
  std::uint32_t alt; // the other buffer's copy of the packet
  std::uint32_t own;
};

// The scratchpad words derived from a command's texture word; returns the page select (0..0x1F0).
std::uint32_t setTexturePage(const psx::present::EmitMemory &memory, std::int32_t word);

// A new slot's packet in both arrays: colours from the corner vertices (in descriptor order), the command byte,
// and for a textured one the CLUT/page words and UVs of the descriptor at `descriptor`.
// Slot `index` of the packet array at `packets`.
struct PacketSlot {
  std::uint32_t index = 0;
  std::uint32_t packets = 0;
};

SlotPair fillNewSlot(const psx::present::EmitMemory &memory,
                     PacketShape shape,
                     std::uint32_t descriptor,
                     const std::array<std::uint32_t, 4> &corners,
                     PacketSlot slot);

// Names the packet by the open object scope; a render over host memory names nothing.
void bindPacket(const psx::present::EmitMemory &memory, std::uint32_t packet);

// The screen points in packet order (P0, P1, P3, P2), and the packet put at the head of the bucket `head`.
void linkPacket(const psx::present::EmitMemory &memory,
                PacketShape shape,
                std::uint32_t packet,
                std::uint32_t head,
                const std::array<std::uint32_t, 4> &xy);

} // namespace ts2
