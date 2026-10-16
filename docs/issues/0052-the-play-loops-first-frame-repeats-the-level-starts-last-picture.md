---
id: 52
title: The play loop's first frame repeats the level start's last picture
status: resolved
symptom: Real-window play check counts repeats=2, one at every level start's hand-off to the play loop
tags: presentation,frame,interpolation
state_items: S003,S011
created: 2026-10-16
updated: 2026-10-16
---

## Observed
Real-window play check (2026-10-16, idle into the attract demo, `warp 0`, fps60 16:9): `records: repeats=2`,
one after the attract level start and one after the warp's level start, every run. The shown record at the play
loop's first two presents was the same (`record 2427 shown again at fence 2428`), and the repeat dropped that
present's in-between.

## Cause
`game/frame/frame_boundary.cpp:CoreFrameBoundary::serviceDeferredDisplay` ran the deferred display `0x80021028`
again in fields whose guest barrier (`game/boot/graphics_sync.cpp:completeOwnedFieldBarrier`, `0x8003FA68`) had
already run it: every field of the level start (its `FieldCall` lives in `ResidentPreparation`, which the old
`fieldCall().pending()` guard did not see) and the first play-loop field after the entry block parks in its barrier.
The level start showed each record in the step it was drawn, the play loop shows the one before, and the switch
showed one record twice.

## Fix
The barrier that ends a host step sets `Session::displayedAtBarrier`; `serviceDeferredDisplay` skips when it is set
or when the level start is suspended. Test: `tests/frame_turn_boundary.cpp`
(`a_field_displayed_at_the_guest_barrier_is_not_displayed_again`). Play check: `repeats=0`.

The level start's display-width switches now show a buffer no record drew for one present each (issue 54).
