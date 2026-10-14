---
id: 23
title: Toy Story 2 product run ignores PSXPORT_NATIVE_FRAMES bound
status: resolved
symptom: PSXPORT_NATIVE_FRAMES=2200 is UNKNOWN at startup and the CPU-active product continues for more than five minutes until its exact PID is stopped
state_items: S013
tags: framework,cvar,automation,bounded-run,runtime
created: 2026-08-26
updated: 2026-10-14
---

The exact psxport `99a42aa3` capability run set `PSXPORT_NATIVE_FRAMES=2200`, but the startup audit
classified it UNKNOWN and the product did not terminate at the requested bound. The operator observed
PID 1972962 remain CPU-active for more than five minutes with ongoing disc/display progress, then
stopped that exact PID through `safekill`; exit status was 137. No exit audit was available after the
kill.

Evidence: `scratch/logs/capabilities-product-99a42aa3.log` records the requested value, startup
classification, continuing LBA/display activity, and absence of a watchdog/fatal termination. The
PID lifetime, CPU activity, exact-PID stop, and exit status are operator observations from that run.

Shared `native_boot.cpp` contains a legacy `cfg_int` read for this variable, so the startup UNKNOWN
result alone cannot prove the general variable is unread: the audit explicitly has a late-read blind
spot. The product behavior proves this invocation was not bounded. Before changing code, establish
whether Toy Story 2 reaches the loop containing that read and query live CVar/legacy-read state after
initialization. A proper fix must make the requested bound govern the actual shipping loop and add a
terminating product-level regression; a timeout or watchdog is not equivalent to a frame budget.

### Resolution (2026-10-14)
The cap is the framework's RunBudget (CVar cv_native_frames, declared, read at boot), and the product now runs through psx::host::TitleSession, which ends the run when it is spent. Evidence: build/verify/bin/toystory2_port with PSXPORT_NATIVE_FRAMES=600 logs 'run budget: 600 fence step(s)', 'Toy Story 2 reached its run budget of 600 step(s)' and the run-end report, exit 0 (scratch/m3/m3d/smoke.log, 2026-10-14). The product-level regression is psxport tests/test_product_host_session.cpp (a session ends on its cap).
