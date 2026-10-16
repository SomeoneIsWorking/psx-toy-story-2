---
id: 55
title: Intro-movie turns present three times
status: open
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

## Gap
Presenting a movie field once (commit without presenting while the movie owns the picture, no replay) changes the
movie path's presents and pacing and needs a real-window look at all four movies; not done here. The vsync misses in
the same movies are psxport issue 0160.
