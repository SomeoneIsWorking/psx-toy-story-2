# Replay interpolation

Toy Story 2's 60 fps in-betweens come from its own draw section run again with its moving inputs blended
(psxport `ReplayFrame`, `docs/plans/replay-interpolation.md` and `docs/presentation.md` there). The guest's draw
code is the only interpolation: the per-drawer state producers, their recorders, renders and emission keys and
the native part and hierarchy bodies are deleted; the native mesh bodies stay for widescreen (issue 42).

## Frame loop

`FrameDriver::stepFrame` → `stepResidentFrame`: the quota's fields, the deferred display `0x80021028` → `0x8003A888`
(draws the OT the previous update built), `present` (seals that record), then the resident update and the SPU.
The resident loop runs at two fields a logic frame, so each interval has one in-between at t = 0.5.

`0x8003A888`, in order: profiler mark, `0x8003DEC4(buffer)` (screen bars into the OT, the env's clear colour),
`0x8003E498` (a VRAM copy, effect modes 1) or `0x8003EFD4` (mode 2, OT links), `0x8002DC2C` (the colour tint into
the OT), DrawOTag of the buffer's OT head (`+0x2B4 + 0x57B * 4`, ClearOTagR), the pad decode `0x8003B33C`, the
buffer swap and ClearOTagR of the next buffer.

## The resident update and its draw section

`0x8007B254` (the play loop; `0x8007B850` when `0x800A10F4` is set, the orbit camera) ends with:

| step | what it does |
|---|---|
| `0x80078780` | logic: palette-cycle counters, timers, exit fade countdown, the fade step `0x800775CC` |
| `0x80074F80` | HUD and pause menu: logic (state machine, sound starts `0x80072C7C`) and HUD packets into the OT |
| `0x8007863C` | builds the draw list `0x800A4298` from the active list `0x800B18A8` plus the player `0x800B2188` |
| `0x80046A88` | the draw-mode packet: **the section starts here** |
| `0x8002A070` | the world pass: camera into the GTE, sky sprites `0x8001FB64`, visibility lists `0x80027724`, world objects `0x8002622C`, actors `0x80025E44` (sorts the draw list, then `0x8002518C` per actor), effects, the VRAM probe `0x80040160`, profiler marks `0x8003E938`, DrawSync `0x80085A54` |

The section (`game/frame/draw_section.*`) is `0x80046A88` (its original; the override samples), `0x8002A070`,
for the orbit update `0x800742C4`, then the display's links `0x8003DEC4`, `0x8003EFD4` (mode 2 only) and
`0x8002DC2C`. It returns the OT head of `facts::kDrawBuffer`; `linked` names that head of either buffer, so
PutDrawEnv's chain stays the record's own. The section links into a table the display cleared, so every replay
rebuilds it; logic linked before the section (HUD from `0x80074F80`) is in the capture as drawn.

Besides drawing, the world pass sorts the draw list in place, poses animated actors (`0x800426E0` writes the part
table `0x800A9618`), steps the OT write cursor `0x800A151C`, consumes a streaming list (`+0x18` of the nearest
area), and calls DrawSync and GetRCnt. The sandbox rolls the writes back. Two calls reach a device on every
frame and are answered inside a replay by title overrides (`game/frame/draw_replay.cpp`): DrawSync returns 0 (a
replay drives no GPU) and the profiler mark does nothing (its table is never drawn and its stores are put back).

## What moves (`game/frame/draw_motion.*`)

| track | words | blend | identity and liveness |
|---|---|---|---|
| camera matrix | `0x1F800394`, the MATRIX `0x8002C848` leaves (x row widened, shake included) | `TransformRule` | one record |
| camera position | `0x1F800374..7C` and `>> 5` at `0x1F800384..8C` | linear | one record |
| camera angles | `0x800A1618` three s16 | 12-bit angles | one record |
| player | `0x800B2188`: position s32 `+0..+8`, angles s16 `+0x0C..+0x10`, animation `+0x12/+0x16`, frames `+0x18/+0x1C` | `ActorMotion` | model `+0x14` as key |
| actors | 64 records of 0x98 at `0x800C2EA0`, same layout | `ActorMotion` | on the active list, in the life `ActorIncarnations` saw last sample; model key |
| posed parts | part records 0x6C at `0x800A9618`: offset s32 `+0..+8`, rotation nine s16 at `+0x58` | `PartMotion` | parts of a model drawn once whose actor has no animation (the player: its update poses them) |

A frame word is 16.16: `0x800426E0` poses between frame `>> 16` and the next by the low half, the last frame
towards frame 0, so it turns round the animation's frame count (`+4 & 0x7FFF` of the animation header, model
table `0x800C7268`). It blends only while its animation index holds.

## Cuts

`FrameCut` (unchanged): a level or area change, the camera placed afresh `0x80065CD0`, a camera source switch at
`0x80068E2C`. The presenter shows S as drawn across them. A frame that runs no resident update, or switches
between the two updates, breaks the pairing (`DrawReplay::endFrame`, `beginUpdate`).

## Not replayed

- `0x80040160` reads eight VRAM pixels back (StoreImage at `0x3C0,0x7F/0x7D`) and writes them again while
  `0x800A1568` is set for the buffer: those intervals refuse and show S as drawn (issue 0061).
- Front end, level start, selection and MEMORY screens run at one field a frame and have no in-betweens.

## Not tracked yet

Particles and effect pools of the world pass (`0x80044C18`, `0x80046E50`, `0x80048214`), moving world objects of
`0x8002622C`, the light matrices `0x800A8360..` and texture scroll step at the logic rate.

## Proof (2026-11-11)

Hidden window, ires 0, fps60, sink 3840x2160, unpaced (`scratch/replay/drive.py`, warp then Cross twice), after
the producers were deleted:

- t = 1 probe (no track written): 6 of 6 in-betweens byte-identical to the real present on Andy's House and on
  the Construction Yard.
- Andy's House running right: the floor seam steps evenly, about 30 px a present; Construction Yard walking and
  turning: camera, Buzz's pose and the scene land between their neighbours; nothing missing.
- Run end: 1006 intervals replayed, 57 refused (all `0x80040160`); camera, player and posed parts pair every
  record; 14 of 2008 (Andy's House) and 17 of 1025 (Construction Yard) actor draws unpaired.
- Leak proof (before the deletion): the same input with fps60 on and off ends in identical 2 MiB of guest RAM.
- Not yet: a paced real-time run on a quiet host.

The native mesh bodies stay (`MeshSubmitters`): with the guest's own split paths the floor pieces past column
512 drop out of the widescreen margins (issue 42).
