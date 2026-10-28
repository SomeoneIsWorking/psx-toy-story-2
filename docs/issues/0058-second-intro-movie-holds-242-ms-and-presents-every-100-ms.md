---
id: 58
title: Second intro movie holds 242 ms at its start and presents every 100 ms
status: open
symptom: 40 s paced boot run, worst present interval at the second intro movie's start
tags: fmv,pacing
created: 2026-10-28
updated: 2026-10-28
---

## Observed
Paced hidden boot run with no input (ires 0, fps60 1, 3840x2160 sink, `PSXPORT_DEBUG=presentseq`,
`scratch/player-bugs-5/paced40-final-seq/run.log`, load 1.7): the first movie returns at 04:51:29.409Z, the FMV
image is published again, and fence 1643 lands 241.83 ms after fence 1642. Fences 1656-1686 then come every
100.35 ms. Before issue 0057's fix the boot reached this movie about 20 s earlier, so earlier 40 s runs passed it
without a log of its intervals.

## Not yet known
Whether 100 ms is the movie's own frame rate and what the 242 ms start costs (overlay publish, stream open, first
decode). Trace it on the console oracle's frames before any change.
