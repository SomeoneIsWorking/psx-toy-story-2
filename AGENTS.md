# Toy Story 2 port

Ports the USA PlayStation release `SLUS_008.93` as a native/Lightrec hybrid over the shared
`external/psxport` framework. Execution policy: the `dynarec-port`, `dynarec-runtime` and
`dynarec-overrides` skills. When this repository is inside the PSX workspace, also read
[`../AGENTS.md`](../AGENTS.md) and [`external/psxport/AGENTS.md`](external/psxport/AGENTS.md).

## Ownership

`ts2::Runtime` composes title behavior. `game/execution/guest_execution.*` is the one title-side
adapter to psxport's typed guest-call, original-call and image-scoped native-dispatch APIs. Input,
finite-frame, graphics sync, scene observation, projection and asset decoding are separate modules
(`docs/codemap.md`).

Native overrides are registered only after the resident image is authenticated and activated.

## Title facts

- The supported executable is 598,016 bytes; identity is in `docs/info/exe-identity.txt`.
- Overlay identity is image generation plus guest address; the images, slots and guest facts are in
  `external/toystory2-decomp/docs/overlays.md` and `docs/notes/title-facts.md` there.
- The finite frame owner never dispatches guest VSync `0x80088628`.

## Migration gates

The first runtime discriminator consumes exact `SLUS_008.93`, executes nonzero Lightrec blocks,
reaches resident and streamed-module code, exercises image-generation invalidation and agrees with an
independent oracle at a bounded checkpoint. Boot, FMV or one captured frame is insufficient.
Representative gameplay then covers player input, pause/unpause, camera movement, module
replacement, coherent rendering/audio and sustained frame progression.

A bounded execution exit preserves guest and host lifecycle invariants. Do not bypass a
polling/service boundary or inflate its cycle budget to make boot progress.

## RE

RE lives in [`external/toystory2-decomp`](external/toystory2-decomp) (ours, MIT): the splat configs, the symbol map
`config/symbols/<image>.txt`, `docs/re-frontier.md`, behaviour notes and prior art. A new finding is committed there
first and the submodule bumped in the port change that uses it; a new address goes into the symbol map
(`docs/plans/0062-addresses-from-the-decomp-symbol-map.md`). Every address comes from Ghidra on the exact executable.
`tools/ram_image.py` builds the RAM image, `tools/ghidra_xref.py` and `tools/re_xref.py` xref a guest
address.

## Commands

There are no unit tests or gates.

- Build: `CXX=clang++ CC=clang uv run --frozen python external/psxport/tools/port/consumer_build.py --build build/agent --target toystory2_port`
- See the game: `tools/headless_run.py` (`--pad` for a recorded replay) against your own disc.
