# 0062 — Guest addresses from the decomp symbol map

Workspace rule (re-harness, 2026-11-13): RE knowledge lives in the decomp repository; the port reads guest addresses
from the decomp's symbol map as generated constants, never a hand copy. Toy Story 2's decomp is
`external/toystory2-decomp` (MIT, ours); its symbol map is `config/symbols/<image>.txt`, splat `symbol_addrs`
format, one file per image (`slus_008.93`, `memory`, `fmv`, `level01`, `level01_1` .. `level10_1`).

Not started: it rewrites address constants across `game/`, which collides with the 60 fps work in flight. Run it
when the tree is quiet, one subsystem per commit.

## Today

- 157 `constexpr ... k<Name> = 0x80xxxxxx` (and `0x1F80xxxx` scratchpad) constants in 33 files under `game/`
  (render 9 files, frame 6, overlay, input and boot 3 each, debug 2, the rest 1), each a hand copy of an address with
  its meaning in a comment. Their names were seeded into the symbol map (2026-11-13), one name per address:
  - where the port spells one address two ways, the map holds one and the others are renamed when generated:
    `0x8007A9E8` `main` (port `GuestMain`, `MainLoopEntry`), `0x800A1670` `ColdStartFlag` (`ColdFrontEnd`, and
    `kPlaybackMode` in `game/boot/guest_main_boot.cpp`, which mislabels it), `0x800A14D8` `PlaybackPhase`
    (`MenuPhase`), `0x80077598` `FadeTo` (`FadeOut`), `0x8003A838` `GraphicsShutdown` (`ShutdownGraphics`),
    `0x8003A780` `BootGraphicsInit` (`kGraphicsInit`, which is `0x8003A218`'s name), `0x800A1108`
    `RigidBucketOffsets` (`kBucketOffsets`, which is `0x800A12F8`'s), `0x800100E4` `StaticMeshSubmitter` and
    `0x80017FF8` `RigidMeshDrawer` (both `kEntry` in their drawer headers), `0x80010000` `SetScreenRect`
    (`ScreenRectPublisherLeaf`), `0x80027AF0` `ObjectCull` (`ObjectCullLeaf`), `0x1F800000` `MeshScratch`
    (`Scratch`);
  - overlay names: `fmv.txt` (`FmvEntry`, `MoviePlayer`), `memory.txt` (`MemoryScreenReset`, `MemoryScreenDispatch`,
    `MemoryScreen`, `LevelResults`).
- 7 `installNativeOverride(core, 0x80..., "<name>", ...)` calls with literal addresses and hand-typed names, plus
  the stock libgte/libgpu/VSync/libcd/pad bindings in `game/runtime/runtime.cpp`.
- About 125 further literals in code: guest calls (`0x8007C278u`, `0x8007B850u`/`0x8007B254u` in
  `game/frame/outer_loop.h`), return sites and code sites.
- Not symbols, and kept out of the map: the slot bases `0x800D12C0`/`0x800D5D20` (the decomp's `docs/overlays.md`
  holds them as the modules' `vram`), RAM bounds `0x80000000`/`0x80200000`, and code sites inside functions
  (`0x8007AE18`/`0x8007AE20` = `main + 0x430`/`+ 0x438`, the slot-release sites `0x80017B58`/`0x80017EF0` and their
  children in `game/render/slot_release.cpp`).

## Generator

One tool in psxport, shared with Tomba! 2 (psx-tomba plan 0065) and Spyro (psx-spyro plan 0251):
`tools/symbol_constants.py` reads splat `symbol_addrs` files and writes one header per image,
`namespace ts2::sym::<image> { inline constexpr std::uint32_t <name> = 0x...u; }`, plus `size` where the map gives
one. A CMake function (`psxport_symbol_constants(<target> IMAGE <name> MAP <file>...)`) generates them under
`build/`; they are never committed.

- Names are emitted as the map spells them; a name we want changed is changed in the decomp.
- Two names at one address, or one name at two addresses in one image, fail the generation.
- Struct field offsets (actor and part records, the world-object instance, gp-relative words) stay in the port
  until the decomp carries the structure in a header under `include/`, then come from it the same way.
- Code sites are written as `<function> + 0xNN` against a map symbol, so the only literal left is the offset.
- Overlay addresses come from the overlay's own namespace; the image is the module identity
  `game/overlay/` already authenticates.

## Order

1. The tool and CMake function exist in psxport once Tomba! 2 or Spyro lands them; build `toystory2_port` with
   headers nothing includes yet.
2. Overrides: the 7 `installNativeOverride` literals and the `runtime.cpp` bindings take the generated constants and
   the symbol's name.
3. Constants, one subsystem per commit: `game/facts`, `game/boot`, `game/frame`, `game/debug`, `game/input`,
   `game/cd`, `game/audio`, `game/overlay`, `game/fmv`, `game/front_end`, `game/widescreen`, `game/render`. Each
   constant becomes a use of the generated name, or the name is added to the decomp map first (decomp commit,
   submodule bump in the same port change).
4. The remaining literals in code, by the same order.
5. Build with Clang; `warp 0` into Andy's House once at the end to show the bindings still fire.
