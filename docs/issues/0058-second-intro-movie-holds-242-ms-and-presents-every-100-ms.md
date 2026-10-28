---
id: 58
title: Second intro movie holds 242 ms at its start and presents every 100 ms
status: closed (retail behaviour)
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

## Oracle verdict, 2026-10-28
The full-console oracle (`psxport tools/oracle/console.py`, SCPH-1001 BIOS, same CHD, no input), captured every
field from 3400 to 4300 (`scratch/player-bugs-6/oracle/fine*/frames.jsonl`, `boundary.png`):
- The first movie (`acti.str`) changes picture every 2 fields to its last change at field 3716, which is black.
- The screen stays black until field 4030, the first `dlogo.str` picture: 314 fields (5.2 s) on the console.
- `dlogo.str` then changes picture every 6 fields (5 or 7 at a few points), 100 ms: 10 frames a second.

The stream says the same: its XA audio clocks the read at 150 sectors a second and each video frame owns 15 sectors
(2340 sectors, 156 frames). So 100 ms between presents is the movie's own rate, and the port's 242 ms between the
first movie's last (black) picture and the second movie's first is black on screen, 5 s shorter than the console:
the first movie's stream tail after its last frame (109 ms), the overlay publish and stream open (32 ms), and the
first frame's 15-sector slot (100 ms).

Shown on HEAD, paced hidden run (ires 0, fps60 1, 3840x2160 sink, `PSXPORT_DEBUG=presentseq`, load 5.3,
`scratch/player-bugs-6/paced-movie`): fence 1643 at 244 ms after 1642, then 1644-1734 every 100.1-100.5 ms; every
other interval in the 45 s is under 40 ms except fence 3 (80 ms, issue 0057's decode).

No change: the movie plays at its source rate, and nothing at its start is a held picture.
