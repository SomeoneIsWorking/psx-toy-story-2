---
id: 61
title: Intervals whose world pass probes VRAM are shown without an in-between
status: resolved
symptom: fps60=1 gameplay runs at 30 fps while the world pass probes VRAM (57 consecutive intervals in the attract demo of Andy's House, nearly every interval of level index 11); the replay log names a device read under 0x8002A070
tags: fps60,render
state_items: S011
created: 2026-11-11
updated: 2026-11-17
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

## Resolution (2026-11-17)
The probe is the world pass's light-glow test (`external/toystory2-decomp/docs/notes/frame-loop.md`, the glow probe).
A level survey through the warp (`scratch/fpv/s1.log`) found it on level index 11 too: 374 of its intervals refused.
`GlowProbe` (`game/render/glow_probe.*`) owns `0x80040160`: the guest's steps natively, StoreImage and LoadImage
through the guest on a real run; in a replay it reads the strip from VRAM, which nothing writes between the replay and
the real run's read, and leaves the write-back to the real run.

Shown (hidden window, ires 0, fps60, 3840x2160 sink, unpaced): boot, the attract demo, `warp 11`, 300 frames
(`scratch/fpv/g1.log`): 1171 intervals replayed, 0 refused. The demo's former window (`scratch/fpv/g3.log`, fences
3513-3547) has a t = 0.5 in-between in every interval, and the table leg in five presents shifts 0, 7, 13, 21, 29 px
(in-betweens at 7 and 21 against midpoints 6.5 and 21; `scratch/fpv/g3demo_leg.png`).
