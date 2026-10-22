---
id: 56
title: Mesh producers drop the GTE's sub-pixel places
status: open
symptom: in-betweens of meshes drawn by the quad/triangle splitters and the static, rigid and part-face drawers snap every vertex to a whole native pixel above 1x
tags: render,fps60
state_items: S011
created: 2026-10-22
updated: 2026-10-22
---

## Observed
Code review, 2026-10-22 (psxport issue 0169): `quad_split.cpp`, `tri_split.cpp`, `static_mesh_drawer.cpp`,
`rigid_mesh_drawer.cpp` and `part_face_drawers.cpp` take `Gte::projected(i).word()` and carry the word through
locals and `MeshCpu::sw`, so the place the projection gave the vertex never reaches the packet. Above 1x their
in-betweens draw on whole native pixels.

`hierarchy_drawer.cpp:link` copied its four `kScreen` words into the packet with `mem_r32`/`mem_w32`, which dropped
the places `rotAverage4` stored. It now copies them with `loadVertex`/`storeVertex`.

## Cause
The drawers transcribe the guest's register flow word by word (`MeshCpu` locals and `sw`); a place travels only
through `EmitMemory::storeVertex`/`loadVertex`.

## Fix (not done)
Each drawer keeps its projected vertices as `gte::ProjectedVertex` from `Gte::projected` to the packet store, computes
split midpoints with a place (psxport `presentation.md`, Sub-pixel place) and stores them with `storeVertex`.
