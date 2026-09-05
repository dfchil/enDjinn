#!/usr/bin/env python3
"""Measure full-screen translucent modifier cost through pc-enDjinn."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import time
from statistics import median


DEFAULT_COUNTS = (0, 12, 48, 256, 1024, 4096)


def measure(binary: Path, triangles: int, frames: int) -> float:
    with tempfile.TemporaryDirectory(prefix="enj-modifier-bench-") as directory:
        screenshot = Path(directory) / "frame.ppm"
        environment = os.environ.copy()
        environment.update({
            "ENJ_MODIFIER_BENCH_TRIANGLES": str(triangles),
            "ENJ_OFFSCREEN_CAPTURE": "1",
            "ENJ_SCREENSHOT_FRAME": str(frames),
            "ENJ_SCREENSHOT_PATH": str(screenshot),
        })
        started = time.perf_counter()
        process = subprocess.Popen(
            [str(binary)], cwd=binary.parent, env=environment,
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
        expected_size = 1280 * 960 * 3
        deadline = started + 90.0
        try:
            while time.perf_counter() < deadline:
                if screenshot.exists() and screenshot.stat().st_size >= expected_size:
                    return (time.perf_counter() - started) * 1000.0 / frames
                if process.poll() is not None:
                    raise RuntimeError(
                        f"benchmark exited early for {triangles} triangles")
                time.sleep(0.01)
            raise RuntimeError(f"benchmark timed out for {triangles} triangles")
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=3.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=3.0)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("binary", type=Path)
    parser.add_argument("--frames", type=int, default=120)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--counts", type=int, nargs="+", default=DEFAULT_COUNTS)
    arguments = parser.parse_args()

    if arguments.frames <= 0:
        parser.error("--frames must be positive")
    if arguments.repeats <= 0:
        parser.error("--repeats must be positive")
    if not arguments.binary.is_file():
        parser.error(f"binary not found: {arguments.binary}")

    binary = arguments.binary.resolve()
    measure(binary, 0, min(arguments.frames, 30))
    for triangles in arguments.counts:
        samples = [measure(binary, triangles, arguments.frames)
                   for _ in range(arguments.repeats)]
        milliseconds = median(samples)
        print(f"{triangles:4d} triangles: {milliseconds:7.2f} ms/frame")


if __name__ == "__main__":
    main()
