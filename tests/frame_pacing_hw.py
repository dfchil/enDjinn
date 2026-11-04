#!/usr/bin/env python3
"""Run tests/apps/frame_pacing on a Dreamcast and check each pacing's timing.

Needs a shell that sourced environ.sh (kos-tool on PATH goes through the
console queue) and DCTOOL_HOST set to a loader-ready console.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys

LINE = re.compile(
    r"FRAMEPACING pacing=(\w+) layers=(\d+) ticks=(\d+) rendered=(\d+) "
    r"dropped=(\d+) late=(\d+) ms=(\d+)")

# 360 ticks at 60 Hz take 6 s; allow for the PAL/NTSC refresh and a slow
# phase start.
REAL_TIME_MS = (5700, 6600)


def check(rows: list[dict]) -> list[str]:
    errors = []

    def row(pacing: str, layers: int) -> dict | None:
        for r in rows:
            if r["pacing"] == pacing and r["layers"] == layers:
                return r
        errors.append(f"no result for {pacing} with {layers} layers")
        return None

    def real_time(r: dict, what: str) -> None:
        if not REAL_TIME_MS[0] <= r["ms"] <= REAL_TIME_MS[1]:
            errors.append(f"{what}: {r['ms']} ms for {r['ticks']} ticks, "
                          f"expected {REAL_TIME_MS[0]}-{REAL_TIME_MS[1]}")

    for pacing in ("STRICT", "DROP_LATE"):
        for layers in (0, 4):
            r = row(pacing, layers)
            if r:
                real_time(r, f"{pacing} {layers} layers")
                # The first frame under a new load may run late once.
                if r["late"] > 2 or r["dropped"] > 2:
                    errors.append(f"{pacing} {layers} layers: late={r['late']} "
                                  f"dropped={r['dropped']} on a light load")
    for layers in (5, 6):
        strict, drop = row("STRICT", layers), row("DROP_LATE", layers)
        if strict and strict["ms"] < REAL_TIME_MS[1] * 1.5:
            errors.append(f"STRICT {layers} layers: {strict['ms']} ms, the "
                          "load does not make frames late")
        if drop:
            real_time(drop, f"DROP_LATE {layers} layers")
            if drop["dropped"] == 0 or drop["dropped"] > drop["ticks"] // 2 + 1:
                errors.append(f"DROP_LATE {layers} layers: dropped="
                              f"{drop['dropped']} of {drop['ticks']}")
    half = row("HALF", 5)
    if half:
        real_time(half, "HALF 5 layers")
        if abs(half["rendered"] - half["ticks"] // 2) > 1:
            errors.append(f"HALF 5 layers: rendered={half['rendered']} of "
                          f"{half['ticks']}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf")
    parser.add_argument("--timeout", type=int, default=300)
    args = parser.parse_args()
    host = os.environ.get("DCTOOL_HOST")
    if not host:
        print("DCTOOL_HOST is not set: source environ.sh first", file=sys.stderr)
        return 2
    try:
        out = subprocess.run(["kos-tool", "-t", host, "-x", args.elf],
                             capture_output=True, text=True,
                             timeout=args.timeout).stdout
    except subprocess.TimeoutExpired:
        print(f"no exit within {args.timeout} s; the console may still run "
              "the test (kos-tool -r resets it)", file=sys.stderr)
        return 1
    rows = []
    for m in LINE.finditer(out):
        rows.append({"pacing": m[1], "layers": int(m[2]), "ticks": int(m[3]),
                     "rendered": int(m[4]), "dropped": int(m[5]),
                     "late": int(m[6]), "ms": int(m[7])})
        print(m[0])
    errors = check(rows)
    for e in errors:
        print("FAIL", e)
    print("frame_pacing_hw:", "FAIL" if errors else "ok")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
