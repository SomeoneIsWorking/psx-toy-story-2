---
id: 42
title: 16:9 margins show gaps at the outer edges
status: open
symptom: Black or dark wedges where floor or wall should continue at the far left and right of the 684-wide canvas
tags: widescreen,render
state_items: S010
created: 2026-10-07
updated: 2026-10-29
---

Seen from boot in every attract level at 16:9 (`scratch/readme-media/survey2.jpg`): the guest's `rgb(33,33,33)`
backdrop shows through ragged, polygon-shaped gaps in both margins, worst at the right (the construction yard, Al's
Toy Barn and the kitchen level; Andy's House too). Every 16:9 gameplay still for the README is held back by it.

## Fixed
The native subdividers (`SplitEngine::storeScreenMask` and the quad/triangle centre test) culled children against
the retail columns 0..511. They now cull against `ResidentWidescreenCull::drawnColumns`, the retail columns plus the
record canvas's margins. Shown on presents 5500 and 6250 of an idle boot at 16:9: the left margin's backdrop share
fell from 22.5% and 13.6% to 0% and 0.1%. 4:3 is the same window as before.

## Remaining cause
An unbounded subdivider window leaves the right-margin gaps unchanged, so they are not the native split. The
remaining tests against the retail columns are guest code the port still runs:
- the static submitter's guest path (`submitOnGuest`, the meshes with no native body, issue 41) calls the guest
  subdividers `0x80014D3C`, `0x80016690`, `0x8001A4D4`, `0x8001B8AC`, each with `sltiu ...,0x200`
  (`0x80014F84`, `0x8001688C`, `0x8001A71C`, `0x8001BAA8`);
- the sprite quads `0x8001FB64` (`sltiu ...,0x200` at `0x8001FF78..0x8001FF9C`, any corner left of 512);
- the actor part drawers' vertex tests (`slti ...,0x201`, `0x80031D80` onwards, issues 43-49).

**Proper fix**: give those calls native bodies (issue 41 for the static meshes, the part-drawer issues, a native
sprite drawer) so each tests `drawnColumns`; then re-measure the margins on the same presents.
