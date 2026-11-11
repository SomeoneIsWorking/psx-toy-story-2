---
id: 61
title: Intervals whose world pass probes VRAM are shown without an in-between
status: open
symptom: fps60=1 gameplay runs at 30 fps while the world pass probes VRAM (57 consecutive intervals in the attract demo of Andy's House); the replay log names a device read under 0x8002A070
tags: fps60,render
state_items: S011
created: 2026-11-11
updated: 2026-11-11
---

## Reproduction
`scratch/replay/drive.py e1 run:4000 warp:0 ...` at the player's settings: the attract demo of level index 0 (fences
3511-3658) refuses 57 intervals in a row, each `the redraw of 0x8002A070 reached device-read 16-bit @0x1F801074`.
Each refused interval used to spin the replay's one-guest-second runaway budget first (about 540 ms a logic frame,
two game-seconds at 2 logic frames a second); since a discarded run stops at its first device access
(psxport `SideEffectMode::Discard`) a refused interval costs about 25 ms unpaced against 16 ms for a replayed one.

## Cause
`0x80040160` (world pass `0x8002A070`), while `0x800A1568 + buffer * 2` is nonzero, StoreImages the eight pixels at
`(0x3C0, 0x7F - 2 * buffer)`, walks `0x80040DB4` for each that reads `0x3E0` or `0x3C0`, writes `0x7FF` back and
LoadImages them. The libgpu queue `0x80087B10` masks interrupts (`0x800889A8`, I_MASK) and waits on GPUSTAT for it,
which a replay cannot reach, so the interval is shown as the guest drew it.

## Proper fix
Find what the pixels encode (they are written by an earlier draw of the same buffer) and give the replay the
value the real frame read: the probe's result is a logic input of that frame, recorded before the section and
served to the replay, or a native `0x80040160` that reads the record instead of VRAM in both paths.
