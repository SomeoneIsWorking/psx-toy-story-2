---
id: 50
title: Hierarchical model drawer 0x8002B6F0 has no native body
status: open
symptom: Primitives from 0x8002B6F0 are drawn as the newer record in a 60 fps in-between
tags: interpolation,render,producers
state_items: S009,S011
created: 2026-10-15
updated: 2026-10-15
---

`0x8002B6F0` (738 instructions, `0x8002B6F0..0x8002C277`) runs as guest code. Compiled C over the helpers `0x80084024`, `0x80083F74` and the subdivider `0x80084154`; about 30k primitives on the Andy's House route, frames 1400..2000. Called from the world pass `0x8002622C` beside `0x8002AC40`, not through `0x8002518C`. Reached on the Andy's House route.

Each packet it writes is drawn unkeyed, so an in-between shows it as the newer record. **Proper fix**: a native byte-exact port in `game/render/` over `EmitMemory` and `core.gte()`, each packet keyed by the world object record the caller holds in s7 (as `HierarchyDrawer` does), with its own recorder and state render saving the subdivider's child packets too; proved by the override differential on every call (0 mismatches) and a `composeFrame` test (t=1 equals the drawn frame, t=0.5 lies between).

Split from issue 40; RE in `docs/re-frontier.md` RE-25.
