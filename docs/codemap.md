# Codemap — Toy Story 2

Where Toy Story 2's code lives and what each owner owns. Feature chains are in `docs/features.md`; the
framework side is `external/psxport/docs/codemap.md`.

Frame loop: owned. `stepResidentFrame` runs the resident loop `0x8007A9E8`'s update and replaces its barrier `0x8003FA68(2)`; front-end screens' barriers publish their requested count (`completeOwnedFieldBarrier`).

## Owners

| owner | path | owns |
|---|---|---|
| `createCatalog` | `game/entry/catalog.*` | The one-title catalog: `SLUS_008.93` identity, `PSXPORT_TS2_DISC`, provisioning under `scratch/bin`, one `ts2::Runtime` |
| `ts2::Runtime` | `game/runtime/runtime.*` | The `GameRuntime`: crt0 (`Crt0Boot`), stock libgte/VSync/libgpu/DMA-callback/libcd/pad objects bound in `registerOverrides`, the title overrides, session, frame driver, packet pool, cut, widescreen policy, input phase, host identity |
| `ts2::Session`, `session` | `game/runtime/session.*` | Per-Core title state: boot owners, overlays, sound banks, pad, movie player, frame cut, actor incarnations, save-state section |
| `ts2::TitleState` | `game/runtime/title_state.*` | The machine save state's `ts2` section: holds nothing; a restore makes the replay forget its samples |
| `GuestMainBoot` | `game/boot/guest_main_boot.*` | The measured prefix of guest main `0x8007A9E8`, then overlay initialization, stopping before the outer loop |
| `GraphicsSync`, `publishFieldBarrier` | `game/boot/graphics_sync.*` | Graphics init, resident init, shutdown and field barrier (`0x8003A218`, `0x80039D9C`, `0x8003A838`, `0x8003FA68`) with no guest-owned VSync |
| `LevelStartPresentation` | `game/boot/level_start_presentation.*` | The `0x8007C278` override per level start; suppresses only the demo-forced LOADING card |
| `createFrameDriver`, `ts2::FrameDriver` | `game/frame/frame_driver.*` | The `FrameDriver`: an armed dev warp, then one `stepResidentFrame` per step |
| `DevWarp` | `game/debug/dev_warp.*` | The level arrival of the warp catalogue (`level N`): legal while a level runs; ends it with the attract demo's exit (reason 4, 46-field fade) and the outer loop starts level cursor `<level>` on the player leg |
| `stepResidentFrame`, `ResidentFrameBoundary` | `game/frame/resident_frame.*` | The per-frame order: quota, logic frame, input, fields, deferred display, present, update, audio |
| `CoreFrameBoundary`, `FrameCallState` | `game/frame/frame_boundary.*` | Both boundaries over one Core; the state a turn carries to the next. One deferred display `0x80021028` per field: the host runs it unless the guest's own barrier did or a guest call is suspended; the SPU advances one field per delivered field |
| `stepOuterLoop`, `OuterLoopState` | `game/frame/outer_loop.*` | One finite outer-loop operation per call: front-end poll field, selection, resident update, transition field; a taken warp replaces the post-level route |
| `ResidentPreparation` | `game/frame/resident_preparation.*` | Level start `0x8007BEC4` and the play-loop entry as two resumable calls |
| `FieldCall` | `game/frame/field_call.*` | A guest call driven across display fields over `psx::cpu::ResumableGuestCall` |
| `DrawReplay` | `game/frame/draw_replay.*` | The 60 fps in-betweens (`docs/plans/replay-interpolation.md`): samples at the draw-mode packet `0x80046A88` of a resident update, owns the `ReplayFrame`; `replaying` gates the host notes of overrides a replay could reach |
| `DrawSection` | `game/frame/draw_section.*` | The replayed section: `0x80046A88`, the world pass `0x8002A070` (and the orbit update's pause menu), then the display's links before DrawOTag |
| `DrawMotion`, `ActorMotion`, `PartMotion` | `game/frame/draw_motion.*` | The section's motion tracks: scratchpad camera, published angles, player, actor pool, part records the update posed |
| `FrameCut` | `game/frame/frame_cut.*` | Whether a sealed record is a cut: level or area change, camera placed afresh (`0x80065CD0`), camera source switch (`0x80068E2C`) |
| `GuestCall`, `callGuestToReturn`, `callFiniteGuestToReturn`, `GuestRegisters`, `guestString` | `game/execution/guest_execution.*` | Typed bounded guest calls, their slice caps, and the guest string reader |
| guest facts | `game/facts/guest_facts.h` | Graphics buffers, OT and packet pool offsets, the VSync(-1) counter, the play loop's exit reason and countdown; evidence in `docs/re-frontier.md` |
| `PadOwner` | `game/input/pad_owner.*` | The guest's pad decode `0x8003AC58`, pad init and shutdown; the slot buffers `0x800CF8A0`/`0x800CF8C8` |
| `InputPhase` | `game/input/recording_phase.*` | The replay key from four guest words that hold still per screen |
| `FileTransfer` | `game/cd/file_transfer.*` | The whole-file read `0x80082608` from the authenticated disc, and its caller's bounded retry |
| `OverlayImages`, `OverlaySlot` | `game/overlay/overlay_images.*`, `overlay_slot.*` | The LEVEL (`0x800D12C0`) and MEMORY/FMV (`0x800D5D20`) slots: authenticate, publish, retire with invalidation |
| `LevelSlotImage`, `SharedSlotImage` | `game/overlay/{level,shared}_slot_image.*` | The retail module tables of the two slots |
| `SoundBankProcessor` | `game/audio/sound_bank.*` | The VAB bank routine `0x8007F108` under the bounded resume loop |
| `GuestMoviePlayer` | `game/fmv/movie_player.*` | The FMV overlay's player `0x800D7088`: one movie frame per turn, the retail return value |
| `MemoryScreen` | `game/front_end/memory_screen.*` | The MEMORY overlay's still screen `0x800D9704`: one field per turn, Start ends it through its own fade |
| `guestWidescreenPolicy` | `game/widescreen/guest_widescreen.*` | 16:9 from the configured aspect |
| `ResidentWidescreenCull` | `game/widescreen/resident_widescreen.*` | Widened visibility window `0x80027AF0` and screen rectangle `0x80010000` |
| `OrderingTables` | `game/render/ordering_tables.*` | Names both OTs so entries carry their bucket |
| `MeshSubmitters` | `game/render/mesh_submitters.*` | Overrides `0x800100E4` and `0x80017FF8`: the native body when it covers the call, else the guest original |
| `MeshDrawers` | `game/render/mesh_drawers.*` | The one entry to the ported mesh bodies: plan, run |
| `StaticMeshDrawer` | `game/render/static_mesh_drawer.*`, `static_mesh_plan.cpp`, `static_mesh_layout.h` | `0x800100E4`, positive-header meshes |
| `RigidMeshDrawer` | `game/render/rigid_mesh_drawer.*` | `0x80017FF8` |
| subdividers | `game/render/mesh_subdividers.*`, `quad_split*`, `tri_split*`, `split_*` | `0x80017B6C` and the clip subdividers over `SplitEngine`, which tests the widened screen columns |
| slot packets | `game/render/slot_packets.*`, `slot_release.*` | Slot fill, link, child release |
| `MeshCpu`, culling | `game/render/mesh_cpu.h`, `mesh_culling.*`, `mesh_scratch.h` | The register-level body over `EmitMemory` and `core.gte()`, scratchpad `0x1F800000` |
| mesh format | `game/render/mesh_format.*`, `mesh_draw_call.h`, `memory_range.*` | Resident mesh layout and command walk; a call's registers and read ranges |
| `ActorIncarnations` | `game/render/actor_incarnation.*` | Actor reset `0x8006A9FC` bumps the object generation |
| launcher | `tools/run.py`, `tools/resolve_disc.py`, `tools/extract_exe.py`, `tools/discdump.py` | Disc resolution and extraction for the zero-argument launch |
| run control | `tools/headless_run.py`, `tools/execution_ledger.py`, `tools/route.py` | Hidden-window runs, replays, the run-end ledger |
| RE evidence | `tools/ram_image.py`, `tools/ghidra_xref.py`, `tools/re_xref.py` | The RAM image Ghidra imports and address xrefs |
| framework fetch | `bootstrap.py` | The `external/psxport` fetch (psxport `tools/psxport_fetch.py`) |
| replays | `replays/` | Player route through Andy's House, pause, area transition, level exit, intro skip, movie viewer, memory card |

## Where new work goes

- A guest function Toy Story 2 replaces → `game/<subsystem>/`, installed from `Runtime::registerOverrides`.
- A stock library body → a psxport stock class bound in `ts2::Runtime`, never a title copy.
- Per-Core title state → `ts2::Session`.
- A native drawer body → `game/render/`, installed as a plain override; interpolation stays the draw section replay.
- A measured guest address → beside the owner that reads it, evidence in `docs/re-frontier.md`.
- Anything every title shares → psxport, not here.
