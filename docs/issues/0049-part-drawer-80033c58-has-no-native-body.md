---
id: 49
title: Type-10 part drawer 0x80033C58 has no native body
status: resolved
symptom: Primitives from 0x80033C58 are drawn as the newer record in a 60 fps in-between
tags: interpolation,render,producers
state_items: S009,S011
created: 2026-10-15
updated: 2026-11-11
---

`0x80033C58` (2458 instructions, `0x80033C58..0x800362BF`) runs as guest code. Part type 10. Reached on the Andy's House route (`replays/player_andys_house_v2.pad`, 2000 frames, `PSXPORT_REACH_REPORT`), so an override differential can prove a port there.

Each packet it writes is drawn unkeyed, so an in-between shows it as the newer record. **Proper fix**: a native byte-exact port in `game/render/` over `EmitMemory` and `core.gte()`, each packet written inside the part object's key, a `PartDrawRecorder` save and the shared `PartStateRender`; proved by the override differential on every call (0 mismatches) and a `composeFrame` test (t=1 equals the drawn frame, t=0.5 lies between).

Split from issue 40; RE in `docs/re-frontier.md` RE-25.

## Resolution (2026-11-11)

Moot: in-betweens are the guest's own draw section replayed with blended inputs (`DrawReplay`, `docs/plans/replay-interpolation.md`), so a drawer needs neither a native body nor per-face identity to move at t. The state producers, their recorders and keys are deleted.
