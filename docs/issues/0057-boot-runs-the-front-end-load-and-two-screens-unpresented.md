# 0057 — Boot runs the front-end load and two screens as one unpresented call

Status: screens fixed (2026-10-28); the decode hold stays open. Owners: `CoreFrameBoundary::stepColdFrontEnd`
(`game/frame/frame_boundary.cpp`) and `MemoryScreen` (`game/front_end/memory_screen.*`).

## Observed
The all-title real-time run reports `worst=501.63ms`, between fence 1 and fence 2, the first two presents after boot.
Every later gap is at most 100 ms. Fence 2 is shown on time against its due, so the host keeps pace; the picture
holds for the whole call.

## Cause
`0x8007BC74(10,0)` stores the fade length, runs graphics init `0x8003A218`, loads the front-end asset set
`0x8003D88C(0)`, then MEMORY `0x800D95C4` arm 10, which is `0x800D9704(2)` then `0x800D9704(3)`. The port runs all of
it as one finite call, so nothing presents until it returns. Measured on a hidden run with `PSXPORT_DEBUG=guest-call`:

1. The asset set: 12 suspended turns (`7,196,427` guest cycles over the call), every one at `pc 0x80021340..0x800216D8`
   with `ra=0x8003B718`, inside DecompressRAW `0x80021190` called from `0x8003B544`. The file reads are the native
   whole-file read (`FileTransfer`), so no CD time is involved: this is the guest's own decode, which retail pays too.
   About 65 ms of host time.
2. The two screens: `0x800D9704(id)` shows graphic `id` and loops `0x8003FA68(1)` (the field barrier), the draw and
   a 600-field countdown that subtracts the elapsed fields `DAT_800A1174`; Start ends it once 300 fields are left (600
   after the first boot, `DAT_800A1670`), a new button press too. In a finite call the native barrier
   (`game/boot/graphics_sync.cpp:completeOwnedFieldBarrier`) publishes the elapsed fields without waiting or
   presenting, so both 600-field loops run to the end inside the last turn: 1,200 iterations and 3,604 ordering-table
   walks, about 270 ms of host time, and neither screen is ever shown.

Neither part is a CD or load wait, so there is no instant-CD removal to apply.

## Fix
`0x8007BC74(10,0)` now runs as a `FieldCall` with `yieldAtFieldBarrier` (`stepColdFrontEnd`, phase `coldRestart`).
`MemoryScreen` owns `0x800D9704` (installed when the MEMORY image is published): retail's loop, one field per host turn
(the barrier `0x8003FA68(1)` publishes through `publishFieldBarrier`, then the turn ends and re-enters at `0x800D9704`).
Its Start skip drops retail's 300-field lock on the first boot and takes the screen's own terminal: countdown to 24,
the 24-field fade `0x80077598(0,0,0,0xC)`, and the retail return value `0x800A1670` (zero on the first boot, so screen
3 still shows). New D-pad presses keep retail's rule.

Shown, hidden paced runs (ires 0, fps60 1, 3840x2160 sink, `PSXPORT_DEBUG=presentseq`):
- No input (`scratch/player-bugs-5/shots-screen-paced`): `cold front-end load returned after 1200 display field(s)`;
  the trademark screen and the ESRB screen are presented (`strip.png`), then the intro movie. Every interval over
  34 ms lines up with a `pshot` capture except fence 3 (63 ms), the decode.
- Start at 2 s and 4 s (`shots-skip`): `returned after 219 display field(s)`; Start ends each screen through its fade.

## Remaining
The front-end decode (fence 3, about 64 ms) is guest CPU work in DecompressRAW `0x80021190`; removing it is a native
decoder with byte-equal output on the whole `.raw` corpus (RE-08), which shortens every level load too.

## Check
`PSXPORT_DEBUG=presentseq` on a paced hidden run: the `cold front-end load returned after N display field(s)` line,
and the fence 1 to fence 3 intervals.
