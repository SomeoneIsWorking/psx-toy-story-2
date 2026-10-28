---
id: 59
title: Resident frames tick a fixed field quota when the update overruns it
status: open
symptom: none observed; found reading the field barrier while reviewing psxport Timing::vsyncFields
tags: pacing,frame
created: 2026-10-28
updated: 2026-10-28
---

## What the code does
`game/frame/resident_frame.cpp:stepResidentFrame` ticks `displayFieldQuota()` fields (2 in the resident update and
the selection screen, 1 elsewhere), publishes that count as the elapsed fields `0x800A1174` and commits it, whatever
guest time the previous update took.

## What retail does
The barrier `0x8003FA68(n)` the native frame owner replaces (`scratch/player-bugs-9/decomp/c/8003FA68.c`; Ghidra reports the body incomplete, so its tail is not trusted): it waits while the VBlank
accumulator `gp+0x7FC` (`0x800A14D4`, incremented by the VBlank callback `0x80039D60`) is below n, then publishes the
accumulator to `0x800A1174` and clears it. An update that runs past n fields therefore publishes the larger count
(a longer game step) and does not wait. After the publish the barrier appears to wait for the deferred display
request `0x800A10F8`, which the next VBlank's `0x80021028` acknowledges; that next VBlank also counts toward the next
frame.

## Not done
The port never publishes more than the quota, so an overrunning update is followed by a frame paced as the quota and
anchored at the guest's time. Applying the retail rule needs the barrier's tail decompiled whole (the deferred-display
wait and where the next frame's count starts), then a span owned by the frame boundary, not a copy of
`Timing::vsyncFields`, whose libetc VSync rule (`max(n, k + 1)`) differs from this accumulator barrier
(publishes `max(n, k)`, then waits for one VBlank). No run has shown a resident update overrunning 2 fields.
