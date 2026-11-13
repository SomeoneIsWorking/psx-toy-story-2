# Replay interpolation

Toy Story 2's 60 fps in-betweens come from its own draw section run again with its moving inputs blended
(psxport `ReplayFrame`, `docs/plans/replay-interpolation.md` and `docs/presentation.md` there). The guest's draw
code is the only interpolation: the per-drawer state producers, their recorders, renders and emission keys and
the native part and hierarchy bodies are deleted; the native mesh bodies stay for widescreen (issue 42).

## Frame loop

`FrameDriver::stepFrame` → `stepResidentFrame`: the quota's fields, the deferred display `0x80021028` → `0x8003A888`
(draws the OT the previous update built), `present` (seals that record), then the resident update and the SPU.
The resident loop runs at two fields a logic frame, so each interval has one in-between at t = 0.5.

The deferred display `0x8003A888`, the resident update and the world pass are described in `external/toystory2-decomp/docs/notes/frame-loop.md`.

## The resident update and its draw section

The section (`game/frame/draw_section.*`) starts after the update's logic, HUD and draw-list build. It is `0x80046A88` (its original; the override samples), `0x8002A070`,
for the orbit update `0x800742C4`, then the display's steps before DrawOTag as `0x8003A888` takes them:
`0x8003DEC4(buffer)`, the effect (`gp+0x588 + gp+0x610 != 0`: the VRAM copy `0x8003E498(buffer)` when either mode
is 1, which refuses, else `0x8003EFD4` for mode 2) and `0x8002DC2C`. The effects and the tint take no arguments. It returns the OT head of `facts::kDrawBuffer`; `linked` names that head of either buffer, so
PutDrawEnv's chain stays the record's own. The section links into a table the display cleared, so every replay
rebuilds it; logic linked before the section (HUD from `0x80074F80`) is in the capture as drawn.

The world pass's writes besides drawing (`external/toystory2-decomp/docs/notes/frame-loop.md`) are rolled back by the sandbox. DrawSync `0x80085A54` is libgpu's stock
wait for both paths (`StockLibgpu`: DMA2 completes at once, so the queue `0x80087B10` drains as it fills); the
profiler mark `0x8003E938` reads root counter 1, which a sandbox holding guest time serves as its held value.
Overrides whose guest call a replay could reach note nothing while `replaying` (`ActorIncarnations`, `FrameCut`).

## What moves (`game/frame/draw_motion.*`)

| track | words | blend | identity and liveness |
|---|---|---|---|
| camera matrix | `0x1F800394`, the MATRIX `0x8002C848` leaves: RotMatrix of the angles, x row `* 512 / 320`, rows 0 and 1 scaled by the shake `gp+0x9C4` | `TransformRule` with scaled rows: the unscaled rotation slerped, the later tick's row scales (widening and shake) | one record |
| camera position | `0x1F800374..7C` and `>> 5` at `0x1F800384..8C` | linear | one record |
| camera angles | `0x800A1618` three s16 | 12-bit angles | one record |
| player | `0x800B2188`: position s32 `+0..+8`, angles s16 `+0x0C..+0x10`, animation `+0x12/+0x16`, frames `+0x18/+0x1C` | `ActorMotion` | model `+0x14` as key |
| actors | 64 records of 0x98 at `0x800C2EA0`, same layout | `ActorMotion` | on the active list, in the life `ActorIncarnations` saw last sample; model key |
| posed parts | part records 0x6C at `0x800A9618`: offset s32 `+0..+8`, rotation nine s16 at `+0x58` | `PartMotion` | parts of a model drawn once whose actor has no animation (the player: its update poses them), while the same actor in the same life poses them |
| sprite frame | `gp+0x898` (`0x800A1570`), the effect pass's sprite frame counter 0..23 | ring of 24 | one record |
| spinners (`SpinnerMotion`, `game/frame/spinner_motion.*`) | the angles `+0x0C..+0x10` of each instance an effect pool record names (byte `+0xC` from `0x30`) | 12-bit angles | the same instance at the same record index |

The pose rule follows the frame words' semantics in `external/toystory2-decomp/docs/notes/frame-loop.md`: a frame word blends round its animation's frame
count only while its animation index holds.

The effect pass `0x80044C18` steps two inputs inside the section by `ElapsedFields`: the sprite frame counter and the
spinners' angles, from which it rebuilds their group matrices. The replay starts from the later sample, so it
blends what the step starts from and the pass adds its own step on top: the in-between lands between the two
real draws. A sprite frame is discrete, so its in-between holds the earlier present's frame.

The section reads the camera twice: the matrix (slerped) and the published angles (lerped, the sky and the screen
effect). Both are exact at the ticks; a turn on two axes at once can put them slightly apart in the in-between.

## Cuts

`FrameCut` (unchanged): a level or area change, the camera placed afresh `0x80065CD0`, a camera source switch at
`0x80068E2C`. The presenter shows S as drawn across them. A frame that runs no resident update, or switches
between the two updates, breaks the pairing (`DrawReplay::endFrame`, `beginUpdate`), and so does a save-state
restore (`TitleState::restored`).

## Not replayed

- `0x80040160` reads eight VRAM pixels back (StoreImage at `0x3C0,0x7F/0x7D`) and writes them again while
  `0x800A1568` is set for the buffer: those intervals refuse at the first GPUSTAT poll and show S as drawn
  (issue 0061).
- Front end, level start, selection and MEMORY screens run at one field a frame and have no in-betweens.

## Not tracked yet

The effect pool's sprite positions and the particle pools `0x80046E50`, `0x80048214`, world objects of `0x8002622C`
that update code turns (`0x8002DFFC`, `0x8002E2A4`, `0x8002E52C`), the light matrices `0x800A8360..` and texture
scroll step at the logic rate.

## Proof (2026-11-11)

Hidden window, ires 0, fps60, sink 3840x2160, unpaced (`scratch/replay/drive.py`):

- t = 1 probe (the replay run with no track written): 22 of 22 in-betweens byte-identical to the real present
  on Andy's House running and standing and on the Construction Yard walking (`scratch/replay/t1a`). The orbit
  update `0x8007B850` was not reached, so its section (`0x800742C4`) has no t = 1 proof.
- Andy's House running right and up (`scratch/replay/e1`): the door's left edge steps 2632, 2608, 2584, 2567,
  2544, 2520 px over six presents (5472 wide), each in-between within 3 px of its neighbours' midpoint; Buzz's
  run cycle lands between its neighbours.
- Run end (`e1`, boot through the attract demo, warp 0, two Cross, run): 1206 intervals replayed, 57 refused (the
  attract demo's VRAM probe, issue 0061), each refusal at the first device read (about 25 ms an interval
  against 16 ms replayed); camera, player and posed parts pair every record, 15 of 2604 actor draws unpaired.
- Leak proof (`leak0`/`leak1`, `PSXPORT_DEBUG=fielddigest`, the same inputs with fps60 off and on, 1246 intervals
  replayed and 57 refused on the on side): every one of the off run's 5080 field digests (RAM hash, I_STAT,
  I_MASK, pad, ticks) appears identically in the on run, which ends on the same tick.
- Paced (2026-11-13, `scratch/replay/paced1.log`, host shared with other agents): 8940 present intervals, 14
  missed vsyncs (0.16%, longest run 3), cadence 0.50 tick per vblank on 4963 vblanks and 1.00 on 1311; the run-end
  1.00 vblanks are all one-field screens (MEMORY, level-start fades, level card, the level start's wait for Cross)
  and every gameplay present advances half a tick (`scratch/replay/h1.log`). Not yet on a quiet host.
- HEAD crops (2026-11-13, `scratch/replay/h1/buzz.png`, `cam.png`): the chair leg in the in-betweens within 3 px
  of its neighbours' midpoint (8 px a native pixel), Buzz's run cycle between its neighbours.
- Effect pass (2026-11-13, idle in Andy's House, `scratch/worldobj/spin1`, `spin3`): the coin sprite above the chair
  showed the next tick's frame in each in-between (widths 10,10,25,25,32,32 px with in-betweens at p0, p2, p4);
  with the sprite frame track each in-between holds the frame before it (7,10,10,25,25,32,32), so the coin turns
  at the guest's 30 Hz. The spinner's angle word steps 20/40 a frame with fps60 off and on alike (no leak), and its
  32674 draws pair; the one spinner in Andy's House (`0x8014FA24`) is out of view at the level start, so its turn
  is not yet seen in frames.

The native mesh bodies stay (`MeshSubmitters`): with the guest's own split paths the floor pieces past column
512 drop out of the widescreen margins (issue 42).
