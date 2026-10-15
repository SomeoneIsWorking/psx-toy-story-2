# Features — Toy Story 2

Title hops only. Each row is where Toy Story 2 joins a psxport feature hop (`psxport F-X.n`); a feature
with no row runs on the framework alone.

## F-BOOT

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-BOOT.1 | `game/entry/catalog.cpp:createCatalog` — `SLUS_008.93` identity, `PSXPORT_TS2_DISC`, one `ts2::Runtime` |
| 2 | psxport F-BOOT.5 | `game/runtime/runtime.cpp:Runtime::startProgram` — crt0 (`kCrt0`), then `GuestMainBoot::initialize`, the measured prefix of guest main `0x8007A9E8` |
| 3 | psxport F-BOOT.6 | `game/runtime/runtime.cpp:Runtime::registerOverrides` — binds `StockLibgte`, `StockVSync` (`0x80088628`, counter `0x8009FD54`), `StockLibgpu`, `StockDmaCallbacks` (`0x8009FD60`) and `GuestInterruptLibcd`, then installs the title overrides |
| 4 | psxport F-BOOT.6 | `game/runtime/session.h:ts2::Session` — per-Core boot owners, overlays, pad, movie player, frame cut, actor state, state producers |

## F-TURN

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-TURN.2 | `game/frame/frame_driver.cpp:FrameDriver::stepFrame` — one `stepResidentFrame` over `CoreFrameBoundary` |
| 2 | psxport F-TURN.2 | `game/frame/outer_loop.cpp:stepOuterLoop` — one finite outer-loop operation per step |
| 3 | psxport F-TURN.2 | `game/frame/field_call.cpp:FieldCall` — a blocking guest call resumed across display fields |

## F-INPUT

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-INPUT.1 | `game/runtime/runtime.cpp:Runtime::deliverPad` — `StockPadBuffers` at `0x800CF8A0`/`0x800CF8C8` |
| 2 | psxport F-INPUT.1 | `game/input/pad_owner.cpp:PadOwner::decode` — the guest's decode `0x8003AC58`, active-low, release `0xFF` |
| 3 | psxport F-INPUT.2 | `game/input/recording_phase.cpp:InputPhase::of` — the title screen's 30-field Start lockout keyed by screen |

## F-CD

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-CD.1 | `game/runtime/runtime.cpp:ts2::Runtime` — `GuestInterruptLibcd`, work area `0x800A0820`/`24`, ready slot `0x800A0808` delivered by the guest's INT1 |
| 2 | psxport F-CD.1 | `game/cd/file_transfer.cpp:FileTransfer::transfer` — whole-file read `0x80082608` from the authenticated disc |
| 3 | psxport F-CD.3 | `game/overlay/overlay_images.cpp:OverlayImages::installLoadObserver` — LEVEL and MEMORY/FMV slots authenticated, published and retired |

## F-RENDER

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-RENDER.1 | `game/runtime/runtime.cpp:Runtime::renderCapabilities` — Record; guest VRAM is the picture |
| 2 | psxport F-RENDER.2 | `game/runtime/runtime.cpp:Runtime::packetPool` — fixed base `0x801BBFEC`, stride `0x214F4` |

## F-FPS60

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-FPS60.1 | `game/runtime/runtime.cpp:Runtime::stateProducer` — answered from `Session::stateProducers` |
| 2 | psxport F-FPS60.1 | `game/render/mesh_draw_state.cpp:MeshStateRender` — static `0x800100E4` and rigid `0x80017FF8` meshes |
| 3 | psxport F-FPS60.1 | `game/render/part_draw_state.cpp:PartStateRender` — actor parts drawn by `0x8001C920` and `0x8001DFE4` |
| 4 | psxport F-FPS60.2 | `game/frame/frame_cut.cpp:FrameCut::isCut` — level, area and camera cuts |

## F-WIDE

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-WIDE.1 | `game/widescreen/guest_widescreen.cpp:guestWidescreenPolicy` — 16:9 from the configured aspect |
| 2 | psxport F-WIDE.2 | `game/widescreen/resident_widescreen.cpp:ResidentWidescreenCull` — visibility window `0x80027AF0` and screen rectangle `0x80010000` widened |

## F-FMV

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-FMV.1 | `game/fmv/movie_player.cpp:GuestMoviePlayer` — the FMV overlay's player `0x800D7088`, Start returns the cold-start word |

## F-SAVE

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-SAVE.1 | `game/runtime/runtime.cpp:Runtime::hostIdentity` — card `PSXPORT_TS2_CARD` (`scratch/saves/toystory2.mcr`) |
