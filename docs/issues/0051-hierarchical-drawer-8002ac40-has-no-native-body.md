---
id: 51
title: Hierarchical model drawer (second form) 0x8002AC40 has no native body
status: resolved
symptom: Primitives from 0x8002AC40 are drawn as the newer record in a 60 fps in-between
tags: interpolation,render,producers
state_items: S009,S011
created: 2026-10-15
updated: 2026-10-15
---

`0x8002AC40` (357 instructions, `0x8002AC40..0x8002B1D3`) ran as guest code, so its packets were drawn unkeyed.

Fixed by `HierarchyDrawer` (`game/render/hierarchy_drawer.*`), a byte-exact port over `EmitMemory` and `core.gte()`. Each quad's packet is keyed by the world object record the caller `0x8002622C` holds in s7 and the quad's index in the model; `HierarchyDrawRecorder` saves the call's inputs and `HierarchyStateRender` replays the body over host copies with the rotation, colour matrix and position blended.

Proof: the override differential (`PSXPORT_OVERRIDE_DIFF=0x8002AC40`, every call, Andy's House route, 2000 frames) compared 1841 calls with 0 mismatches; the census shows 1658 keyed primitives from it, none unkeyed and none without a render; `tests/hierarchy_producer_boundary.cpp` checks the keys, the render at t=1 over scrambled memory, and t=0.5 against a frame drawn with the pose halfway.
