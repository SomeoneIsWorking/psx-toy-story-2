#pragma once

class Core;

namespace ts2 {

// The world pass's glow probe 0x80040160: 0x80040400 draws each light's probe pixel into a VRAM strip, and while the
// strip's flag for the buffer is set the probe reads its eight pixels back, draws the glow 0x80040DB4 of each one the
// light reached (0x3E0 or 0x3C0), and clears them to 0x7FF. A replay reads the strip from VRAM without libgpu's
// StoreImage, which waits on the device, and leaves the write-back to the real run.
class GlowProbe {
public:
  static void install(Core &core);
  static void run(Core &core);
};

} // namespace ts2
