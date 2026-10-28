---
id: 59
title: Resident frames tick a fixed field quota when the update overruns it
status: open
symptom: none observed; found reading the field barrier while reviewing psxport Timing::vsyncFields
tags: pacing,frame
created: 2026-10-28
updated: 2026-10-28
---

## What the code does
`game/frame/resident_frame.cpp:stepResidentFrame` ticks `displayFieldQuota()` fields (2 in the resident update and
the selection screen, 1 elsewhere), publishes that count as the elapsed fields `0x800A1174` and commits it, whatever
guest time the previous update took.

## What retail does
The barrier `0x8003FA68(n)`, read whole from its disassembly (`tools/disasm.py` over a RAM image built from the
EXE; Ghidra's decompile lacks the tail). Inside EnterCriticalSection/ExitCriticalSection
(`0x80089414`/`0x80089424`, syscalls 1 and 2) it waits while the VBlank accumulator `gp+0x7FC` (`0x800A14D4`) is
below n, publishes the accumulator to `0x800A1174` and clears it, clamps the published count to 4, sets the deferred
display request `0x800A10F8` to 1 and spins until the next VBlank's `0x80021028` clears it. That VBlank also counts
toward the next frame, so a steady loop still releases every n fields. Callers: the resident loop `0x8007AEAC`
(`li a0,2`; in attract playback `0x800A120C` the loop then overwrites the count with 2) and `0x8007C2D4`/`0x8007C3D8`
(n = 1).

## Not resolved by owning the loop
Committing only the requested count does not shorten an overrunning frame: the pacer ends an interval at the guest's
time, and Crash Bash showed the fixed count leaving the VBlank callbacks and SPU advances behind the guest clock with
every later field off the grid (psx-crash-bash issue 0045). Retail publishes the fields that elapsed, clamped to 4.
Whether the resident loop's overrunning frames leave the field grid here has not been measured.
