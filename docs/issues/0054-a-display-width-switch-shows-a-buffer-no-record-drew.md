---
id: 54
title: A display-width switch shows a buffer no record drew
status: open
symptom: Four presents without an in-between at each level start, two per display-width switch
tags: presentation,interpolation
state_items: S011
created: 2026-10-16
updated: 2026-10-16
---

## Observed
Real-window play check (2026-10-16, after issue 52): after `warp 0`, `without_in_between` grows by 4 at the level
start: fence 2345 (display `320,256 320x240`, no shown record), 2346 (no `from`), 2440 (`0,256 512x240`, no shown
record), 2441 (no `from`). The attract level start adds the same four.

## Traced
The level start displays 320-wide buffers at `0,256`/`320,256` and the play loop 512-wide ones at `0,256`/`512,256`.
With one deferred display per field the display shows the buffer the previous field drew, so the first field after a
width switch shows a rect no held record drew at that geometry. `FramePresenter::shownRecord` finds no record,
the present is the device's VRAM, and the next present has no `from` to blend.

## Gap
Whether the retail level start draws the newly displayed buffer before the switch is not established; if it does
not, the stale field is faithful and these presents are cuts the counter cannot tell from a dropped in-between.
