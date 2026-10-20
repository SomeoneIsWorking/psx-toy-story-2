#!/usr/bin/env python3
"""Build and launch the Toy Story 2 native/Lightrec product."""

from __future__ import annotations

import os
from collections.abc import Sequence
from pathlib import Path

from port import launcher
from port.launch_environment import player_environment

ROOT = Path(__file__).resolve().parents[1]
DISC_VARIABLE = "PSXPORT_TS2_DISC"


def launch(disc: str | None, prepare_only: bool) -> None:
    launcher.preflight()
    build = launcher.build_product(ROOT, "toystory2_port", os.environ)
    if prepare_only:
        launcher.say("Toy Story 2 is built and ready.")
        return
    env = player_environment(os.environ, product="toystory2")
    if disc:
        env[DISC_VARIABLE] = disc
    launcher.exec_product(build / "bin" / "toystory2_port", [], env, ROOT)


def main(argv: Sequence[str] | None = None) -> int:
    args = launcher.argument_parser(__doc__).parse_args(argv)
    return launcher.main(lambda: launch(args.disc, args.prepare_only))


if __name__ == "__main__":
    raise SystemExit(main())
