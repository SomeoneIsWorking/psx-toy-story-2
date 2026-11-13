# Prior art — Toy Story 2 (USA `SLUS_008.93`, SHA-1 `f90c9cd6b4fc9845adfe34e306b7df393bf9154c`, `docs/info/exe-identity.txt`)

There is no decompilation of this executable; Toy Story 2 needs our own MIT decomp repository. Rows checked through
the GitHub API on 2026-11-13 (licences are GitHub's SPDX ids; "none" means no licence file GitHub recognises). A
borrowed address is a lead until measured on our image.

## PlayStation `SLUS_008.93`

| project | URL | kind | licence | target build, how matched | what we may take | verified features (file) | last push |
|---|---|---|---|---|---|---|---|
| PeriBluGaming/ToyStory2Recomp | https://github.com/PeriBluGaming/ToyStory2Recomp | psxrecomp static recompilation | NOASSERTION (own code PolyForm-Noncommercial-1.0.0, `LICENSE`, SPDX headers) | USA `SLUS-00893`: `game.toml` entry `0x80082D60` equals our header; text `0x92000` is our `0x91800` rounded up; 124 addresses from our `docs/` and `game/` are among its 658 JAL-target seeds | nothing (runtime and design excluded); its comments name addresses we may re-measure | widescreen clip-rect widening at `SetClipRect 0x80010000` (`mods/src/widescreen.c`); intro skip through state word `0x800A1374` in `main 0x8007A9E8` (`mods/src/skip_startup_intro.c`); `mods/src/modern_camera.c` | 2026-11-12 |
| PeterDelta/PCSX2 (`.Playstation PS1/SLUS-00893.cht`) | https://github.com/PeterDelta/PCSX2 | emulator cheat patches | none | USA `SLUS-00893` per the file header; not address-checked | evidence only | 60 FPS GameShark code: `8007AEB0 0002` (held by `A707AEB0 00020001`), gated on `D009A6C4 0039` and `D00A9FD8 7EDC` (level 12) | 2026-11-10 |
| TravellersTalesPSXCollisionViewer | https://github.com/juanmv94/TravellersTalesPSXCollisionViewer | savestate collision/GFX viewer (JS) plus `.RAW` tools | none (all rights reserved) | TS2 retail (`game=ts2`): CLUT/texture-page table `0x800CD200` equals our measured `kTexturePages` (`game/render/slot_packets.h`); collision area at savestate offset `0xb54bb0` → `0x800C9510`, 89 `lui`/`lo` pairs in our image | evidence only | | 2024-01-23 |
| Toy-Story-2-Modding | https://github.com/mouksx/Toy-Story-2-Modding | file-format notes and extracted assets | none | retail data files, no addresses | evidence only (`.NGN` notes); holds extracted game assets, never copy | | 2024-07-05 |

`RAWdec` in the collision viewer embeds and executes x86 decoder bytes and ships `.exe` files: never run.

## Other builds (same Traveller's Tales engine)

The 1999 Windows release is a conversion of the PlayStation game (ToyStory2Web `docs/FORMATS.md`: the PC disc ships
the PSX source data tree), so its formats and gameplay constants are leads for ours; its addresses are not.

| project | URL | kind | licence | target build | what we may take | last push |
|---|---|---|---|---|---|---|
| BMoradi1/ToyStory2Web | https://github.com/BMoradi1/ToyStory2Web | browser reimplementation of the engine | MIT | PC `toy2.exe`; README says constants were checked against the PlayStation build | copy with attribution: format parsers and constants, each re-measured on `SLUS_008.93` | 2026-10-25 |
| woodys-workshop/toy2-pc-decomp | https://github.com/woodys-workshop/toy2-pc-decomp | decomp of the PC executable (VC++ 6, 91 `.c`/`.cpp`) | none | PC `toy2.exe` | evidence only (engine structure, names) | 2026-11-08 |
| opents2-project/OpenToyStory2 | https://github.com/opents2-project/OpenToyStory2 | Rust engine rewrite from Ghidra RE (formerly asmerdon/toy-story-2-rust-rewrite) | none | PC `toy2.exe` | evidence only (`docs/formats/`, `docs/re/`) | 2026-11-12 |
| RibShark/ToyStory2Fix | https://github.com/RibShark/ToyStory2Fix | PC widescreen/framerate/render-distance patch | NOASSERTION | PC, all regions | evidence only | 2018-03-16 |
| Gats1212/ToyStory2Coop | https://github.com/Gats1212/ToyStory2Coop | PC LAN co-op ASI plugin | MIT | PC `toy2.exe` | copy with attribution where its sync design fits | 2026-10-12 |
| ToyStory2Resources | https://github.com/cheeseandcereal/ToyStory2Resources | autosplitter, Cheat Engine tables, Project64 cheats | none | PC and N64, no PlayStation addresses | nothing | 2026-11-09 |

Not found (2026-11-13): a decompilation, splat config, symbol map, Ghidra/IDA database or native port of
`SLUS_008.93` or any other PlayStation region on GitHub, decomp.dev or the web. Searched GitHub repositories ("toy
story 2", "toystory2", "buzz lightyear to the rescue", serials) and the web for decomps, widescreen and 60 fps patches.
