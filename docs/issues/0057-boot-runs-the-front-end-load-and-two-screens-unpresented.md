# 0057 — Boot runs the front-end load and two screens as one unpresented call

Status: open (2026-10-28). Owner: `CoreFrameBoundary::restartColdFrontEnd` / `callMemoryDispatcher`
(`game/frame/frame_boundary.cpp`), which runs `0x8007BC74(10,0)` as one finite call.

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

## Fix (not done)
Run `0x8007BC74(10,0)` as a field-spanning call (the `FieldCall` the front-end poll uses, `yieldAtFieldBarrier`), so
each barrier is one presented field and the two screens appear as on the console. That restores up to 2 x 600
fields of retail screens at boot, so it needs the title-owned Start skip for `0x800D9704` (the countdown is a register,
so the skip owns the function) in the same change. The decode hold that remains is guest CPU work; removing it is a
native `0x80021190` (byte-equal output on the whole `.raw` corpus, RE-08), which shortens every level load too.

## Check
`PSXPORT_DEBUG=guest-call,presentseq` on a paced hidden run: the `cold front-end asset load suspended` lines name the
decoder, and the fence 1 to fence 2 interval is the hold.
