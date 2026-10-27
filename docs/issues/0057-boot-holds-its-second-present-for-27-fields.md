# 0057 — Boot holds its second present for 27 fields

Status: open (2026-10-27). Owner: the guest call `0x8007BC74` at boot (not yet traced).

## Observed
The all-title real-time run reports `worst=481.36ms` (and 487.60 ms, 500.41 ms in repeats) for the present interval.

## Classification
A paced run with `PSXPORT_DEBUG=presentseq` (`scratch/player-bugs/worst`): the worst gap is between fence 1 and fence 2,
the first two presents after boot. Fence 2 was due 467 ms after fence 1 and was shown 4.8 ms late, so the host kept
pace; the gap is guest time. In it the guest call `0x8007BC74` (returning to `0x8007A9E8`) ran for 27 host turns
(15,245,664 guest cycles) before the game presented again. Every later gap is at most 242 ms (fence 440 to 442) and is
also on time against its due.

So it is a boot hold in the guest, not a host stall.

## Next step
Decompile `0x8007BC74` to name what it waits on; if it is a load, it belongs to the loading-removal work.
