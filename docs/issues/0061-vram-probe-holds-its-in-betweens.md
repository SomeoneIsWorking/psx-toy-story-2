---
id: 61
title: Intervals whose world pass probes VRAM are shown without an in-between
status: open
symptom: fps60=1 gameplay shows some logic frames twice; the replay log names budget-exhausted at 0x80087C04
tags: fps60,render
state_items: S011
created: 2026-11-11
updated: 2026-11-11
---

## Reproduction
`scratch/replay/drive.py` at the player's settings, warp 0 or 3, Cross twice: the run-end line reports 57 of
1063 intervals refused, each `the redraw of 0x8002A070 stopped: budget-exhausted at 0x80087C04`.

## Cause
`0x80040160` (world pass `0x8002A070`), while `0x800A1568 + buffer * 2` is nonzero, StoreImages the eight pixels at
`(0x3C0, 0x7F - 2 * buffer)`, walks `0x80040DB4` for each that reads `0x3E0` or `0x3C0`, writes `0x7FF` back and
LoadImages them. The libgpu queue `0x80087B10` waits on GPUSTAT for it, which a replay cannot reach, so the
interval is shown as the guest drew it. Each refusal spins the replay's one-second guest budget first.

## Proper fix
Find what the pixels encode (they are written by an earlier draw of the same buffer) and give the replay the
value the real frame read: the probe's result is a logic input of that frame, recorded before the section and
served to the replay, or a native `0x80040160` that reads the record instead of VRAM in both paths.
