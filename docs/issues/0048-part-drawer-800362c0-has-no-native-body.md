---
id: 48
title: Type-6 part drawer 0x800362C0 has no native body
status: open
symptom: Primitives from 0x800362C0 are drawn as the newer record in a 60 fps in-between
tags: interpolation,render,producers
state_items: S009,S011
created: 2026-10-15
updated: 2026-10-15
---

`0x800362C0` (2767 instructions, `0x800362C0..0x80038DFB`) runs as guest code. Part type 6. Not reached on the Andy's House route (`replays/toystory2_player_andys_house_v2.pad`, 2000 frames, `PSXPORT_REACH_REPORT`); a port needs a title warp to content that draws this part type before a differential can prove it.

Each packet it writes is drawn unkeyed, so an in-between shows it as the newer record. **Proper fix**: a native byte-exact port in `game/render/` over `EmitMemory` and `core.gte()`, each packet written inside the part object's key, a `PartDrawRecorder` save and the shared `PartStateRender`; proved by the override differential on every call (0 mismatches) and a `composeFrame` test (t=1 equals the drawn frame, t=0.5 lies between).

Split from issue 40; RE in `docs/re-frontier.md` RE-25.
