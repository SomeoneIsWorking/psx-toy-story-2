---
id: 53
title: Two-field frames ran the SPU at half rate
status: resolved
symptom: Gameplay audio produced 22,050 samples/s of 44,100 and the XA ring overflowed
tags: audio,frame
state_items: S003
created: 2026-10-16
updated: 2026-10-16
---

## Observed
Real-window play check (2026-10-16): `audio_rate` 22,050 samples/s in Andy's House and the level select (44,100 at
the title), and `xa:warn ring OVERFLOW ... the SPU is not draining CD audio` during the level's music.

## Cause
`game/frame/frame_boundary.cpp:CoreFrameBoundary::advanceAudio` called `SpuAudio::frame`, which advances the SPU by
one display field, once per logic frame, so frames of two fields ran the SPU at half rate.

## Fix
`advanceAudio` advances one SPU field per delivered field. Test: `tests/frame_turn_boundary.cpp`
(`a_two_field_frame_advances_the_spu_two_fields`). Play check: 44,100 samples/s in gameplay, no XA overflow,
`reprimes=0 drops=0`.
