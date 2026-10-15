---
id: 43
title: Dcpl-lit part drawer 0x8001CD34 has no native body
status: open
symptom: Primitives from 0x8001CD34 are drawn as the newer record in a 60 fps in-between
tags: interpolation,render,producers
state_items: S009,S011
created: 2026-10-15
updated: 2026-10-15
---

`0x8001CD34` (323 instructions, `0x8001CD34..0x8001D23F`) runs as guest code. Part type 1, mode 0, called from the object renderer `0x8002518C`. No replay reaches it (Andy's House player, pause, area transition, exit level, intro, movie viewer; confirmed by `PSXPORT_REACH_REPORT` over the Andy's House route), so a port has no differential to prove it. It needs a title warp, modelled on Tomba 2's `warp`, to content that draws a type-1 mode-0 part first; TS2's level selection (`FrameBoundary` level playback, the queued screen) is not yet reverse-engineered far enough to arm one.

Each packet it writes is drawn unkeyed, so an in-between shows it as the newer record. **Proper fix**: a native byte-exact port in `game/render/` over `EmitMemory` and `core.gte()`, each packet written inside the part object's key, a `PartDrawRecorder` save and the shared `PartStateRender`; proved by the override differential on every call (0 mismatches) and a `composeFrame` test (t=1 equals the drawn frame, t=0.5 lies between).

Split from issue 40; RE in `docs/re-frontier.md` RE-25.
