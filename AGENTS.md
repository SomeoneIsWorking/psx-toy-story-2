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
- The executable plus 22 loaded code modules form the code corpus. The four-byte LEVEL00
  placeholder is not code.
- LEVEL alternatives reuse `0x800D12C0`; MEMORY and FMV reuse `0x800D5D20`; FMV enters at
  `0x800D6628`. Overlay identity is image generation plus guest address.
- The first resident call boundary is `0x80089344`; IRQ resume `0x80088A2C` is a mid-function
  re-entry, not a function start.
- Model-table reset `0x80041F38` includes the delay-slot increment at `0x80041FFC`.
- Retail pad buffers are `0x800CF8A0` and `0x800CF8C8`; active-low Cross is `0xBF` and release is
  `0xFF` in the measured packet byte.
- Authored projection leaves are `0x80083CD4` and `0x80083CF4`, initialized to `256/120/160`.
- The finite frame owner never dispatches guest VSync `0x80088628`.
- `.RAW` payloads use Traveller's Tales' `DecompressRAW` LZ format (not RNC); a 14-byte chunk header
  keeps both CRCs, and it decodes the whole retail corpus to exact lengths.

## Migration gates

The first runtime discriminator consumes exact `SLUS_008.93`, executes nonzero Lightrec blocks,
reaches resident and streamed-module code, exercises image-generation invalidation and agrees with an
independent oracle at a bounded checkpoint. Boot, FMV or one captured frame is insufficient.
Representative gameplay then covers player input, pause/unpause, camera movement, module
replacement, coherent rendering/audio and sustained frame progression.

A bounded execution exit preserves guest and host lifecycle invariants. Do not bypass a
polling/service boundary or inflate its cycle budget to make boot progress.

## RE

There is no decompilation of this game; every address comes from Ghidra on the exact executable.
`tools/ram_image.py` builds the RAM image, `tools/ghidra_xref.py` and `tools/re_xref.py` xref a guest
address.

## Commands

There are no unit tests or gates.

- Build: `CXX=clang++ CC=clang uv run --frozen python external/psxport/tools/port/consumer_build.py --build build/agent --target toystory2_port`
- See the game: `tools/headless_run.py` (`--pad` for a recorded replay) against your own disc.
