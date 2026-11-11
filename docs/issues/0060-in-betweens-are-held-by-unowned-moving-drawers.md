---
id: 60
title: Every gameplay in-between is held because unowned drawers move
status: resolved
symptom: fps60=1 gameplay shows each guest frame twice; no in-between carries motion
tags: fps60,render
state_items: S011
created: 2026-10-29
updated: 2026-11-11
---

## Reproduction
Idle boot into the attract rotation, 4:3, `fps60=1`, `PSXPORT_DEBUG=fps60dump`, `PSXPORT_FPS60_DUMP_FROM=4000`, 130
fences of the Andy's House demo: all 130 in-betweens are byte-identical to the newer real present
(`scratch/readme-media/runs/map43`). The run log's `[owners]` rows name the cause: the ownership verdict finds
`UnownedMoving` entries in every interval, and `FramePresenter` then holds the whole interval.

## Unowned moving drawers on the attract route
Static meshes on the guest path through the guest subdividers `0x80014D3C`, `0x80016690`, `0x8001A4D4`, `0x8001B8AC`
(issue 41); part drawers `0x80031C3C`, `0x80033C58`, `0x8003925C`, `0x800362C0`, `0x8001D240` (issues 44-49);
hierarchical drawer `0x8002B6F0` (issue 50); and drawers with no issue yet: `0x8001FB64` (sprite quads), `0x8002B1D4`,
`0x8002C278`, `0x8002DC2C`, `0x80040400`, `0x80040DB4`, `0x8004398C`, `0x80044C18`, `0x80046278`, `0x8004676C`, the
text/HUD writers `0x80085080`, `0x800850A8`, `0x800851E8`, `0x8008491C`, and `0x800872C8`.

## Proper fix
A state producer (or a draw-site tag with an explicit id) for each of those drawers, so the verdict finds no unowned
motion and the composer blends the interval. The README's interpolation clip waits on this.

## Resolution (2026-11-11)

The in-betweens no longer come from keyed records. `DrawReplay` runs the resident update's draw section (`0x80046A88`, the world pass `0x8002A070`, the display's links) again in the sandbox with the camera, player, actor and posed-part tracks blended to t = 0.5, so every drawer, owned or not, draws at t (`docs/plans/replay-interpolation.md`). On Andy's House and the Construction Yard 1006 of 1063 intervals replay; the rest are the VRAM probe of issue 61.
