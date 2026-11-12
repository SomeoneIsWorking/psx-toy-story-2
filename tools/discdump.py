#!/usr/bin/env python3
"""Locate (and if needed build) the framework `discdump` tool and read a disc.

Every disc read goes through psxport's discdump (external/psxport/tools/discdump.cpp). The framework
checkout is $PSXPORT_DIR (default: the sibling checkout); $PSXPORT_DISCDUMP overrides with a prebuilt
binary. psxport's launcher builds it into this repo's build/player-tools/. No file list is cached.
"""
import os
import subprocess
import sys
from pathlib import Path

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def psxport_dir():
    d = os.environ.get("PSXPORT_DIR") or os.path.join(ROOT, "external", "psxport")
    if not os.path.isabs(d):
        d = os.path.join(ROOT, d)
    return d


def find():
    """Return an absolute path to a usable `discdump`, or raise SystemExit(2) saying why not."""
    override = os.environ.get("PSXPORT_DISCDUMP")
    if override:
        if not os.access(override, os.X_OK):
            print(f"[discdump] $PSXPORT_DISCDUMP={override} is not executable", file=sys.stderr)
            raise SystemExit(2)
        return os.path.abspath(override)

    px = psxport_dir()
    if not os.path.isfile(os.path.join(px, "tools", "port", "launcher.py")):
        print(f"[discdump] PSXPORT_DIR={px} is not a psxport checkout; set PSXPORT_DIR.", file=sys.stderr)
        raise SystemExit(2)
    sys.path.insert(0, os.path.join(px, "tools"))
    from automation.process import ToolError
    from port.launcher import build_discdump
    try:
        return str(build_discdump(Path(ROOT), os.environ))
    except ToolError as error:
        print(f"[discdump] {error}", file=sys.stderr)
        raise SystemExit(2) from error


def listing(disc, dd=None):
    """Every file on the disc, as a list of (path, lba, size). Raises SystemExit(2) on a bad read."""
    dd = dd or find()
    out = subprocess.run([dd, "list", disc], capture_output=True, text=True)
    if out.returncode != 0:
        print(f"[discdump] list failed on {disc}:\n{out.stdout}{out.stderr}", file=sys.stderr)
        raise SystemExit(2)
    # `discdump list` prints each file with its full path already ("ARC/PL00_U.ARC   LBA 1100   796672 bytes"),
    # so the header directory must not be prepended again.
    files = []
    for line in out.stdout.splitlines():
        s = line.strip()
        if not s or s.startswith("disc:") or s.startswith("root dir") or (s.endswith("/") and " " not in s):
            continue
        parts = s.split()
        if len(parts) >= 5 and parts[1] == "LBA":
            files.append((parts[0], int(parts[2]), int(parts[3])))
    if not files:
        print(f"[discdump] list produced ZERO files for {disc} — refusing to report an empty disc "
              "as a successful read", file=sys.stderr)
        raise SystemExit(2)
    return files


def get(disc, path_on_disc, outdir, dd=None):
    """Extract one file. `path_on_disc` uses forward slashes ('ARC/PL00_U.ARC'), exactly as
    `discdump list` prints it — the backslash form does NOT resolve. Returns the written path."""
    dd = dd or find()
    os.makedirs(outdir, exist_ok=True)
    out = subprocess.run([dd, "get", path_on_disc, disc, outdir], capture_output=True, text=True)
    dest = os.path.join(outdir, os.path.basename(path_on_disc))
    if out.returncode != 0 or not os.path.isfile(dest):
        print(f"[discdump] get {path_on_disc} failed:\n{out.stdout}{out.stderr}", file=sys.stderr)
        return None
    return dest


# CLI. `list` refuses (exit 2) rather than print an empty listing: discdump missing, disc unresolvable
# or zero entries each exit non-zero naming which.
def _main(argv):
    if not argv or argv[0] not in ("list", "get"):
        print("usage: discdump.py list [disc]\n"
              "       discdump.py get <PATH/ON/DISC> <outdir> [disc]\n"
              "\nThe disc is resolved by tools/resolve_disc.py (CLI arg > $PSXPORT_TS2_DISC > .env > a "
              "*.chd in the repo root).", file=sys.stderr)
        return 2
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from resolve_disc import resolve
    if argv[0] == "list":
        disc = resolve(argv[1] if len(argv) > 1 else None, verbose=True)
        files = listing(disc)                     # refuses on zero
        total = 0
        for path, lba, size in files:
            total += size
            print(f"{path:<32} LBA {lba:<8} {size:>12} bytes")
        print(f"\n[discdump] {len(files)} files, {total} bytes total, on {disc}")
        print("[discdump] DENOMINATOR: this is every entry discdump's ISO9660 walk returned. It says "
              "nothing about what is INSIDE any of them.")
        return 0
    if len(argv) < 3:
        print("usage: discdump.py get <PATH/ON/DISC> <outdir> [disc]", file=sys.stderr)
        return 2
    disc = resolve(argv[3] if len(argv) > 3 else None, verbose=True)
    dest = get(disc, argv[1], argv[2])
    if not dest:
        return 2
    print(dest)
    return 0


if __name__ == "__main__":
    sys.exit(_main(sys.argv[1:]))
