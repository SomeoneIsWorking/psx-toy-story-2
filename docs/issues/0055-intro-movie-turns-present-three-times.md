---
id: 55
title: Intro-movie turns present three times
status: fixed
symptom: 1,356 presents without an in-between and a third of the whole run's intervals during the intro movies
tags: fmv,presentation
state_items: S007,S011
created: 2026-10-16
updated: 2026-10-16
---

## Observed
Real-window play check (2026-10-16): fences 1 to 1355 (the four intro movies) each count `without_in_between` with no
shown record, and every movie frame is three presents: the movie's own (`Fmv::presentFrame`), the frame's commit
~5 ms later, and `Fmv::replayLastFrame` ~0.1 ms after that.

## Traced
`game/frame/frame_boundary.cpp:CoreFrameBoundary::present` commits the guest frame through `FramePresenter` while a
movie is on screen, then replays the movie over it. The guest draws no record during a movie, so each commit
presents device VRAM with no in-between, and the replay re-presents the movie image.

## Cause
`game/frame/frame_boundary.cpp:CoreFrameBoundary::present` committed the guest frame through `FramePresenter` (paced
and presented device VRAM) while the movie owned the picture, then replayed the movie frame over it with
`Fmv::replayLastFrame`.

## Fix
While `Fmv::presenting()`, the boundary advances its delivered guest fields through `Timing::advanceDisplayFields`
and rotates the fence with `FramePresenter::commitUnpresented`; the movie's own paced present is the field's only
present (psxport issue 0160 paces it). `Fmv::replayLastFrame` is removed. Test:
`tests/frame_turn_boundary.cpp` `a_frame_under_a_movie_presents_nothing_of_its_own` (fails with the commit path: one
present).

Real-window check (2026-10-16): one present per movie frame (1,347 movie frames, 1,347 movie presents), the run's
`records: without_in_between=5` (was 1,356), missed vsyncs in the four movies 1 (was 473).
