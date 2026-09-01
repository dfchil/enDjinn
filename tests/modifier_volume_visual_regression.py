#!/usr/bin/env python3
"""Deterministic pc-enDjinn modifier-volume framebuffer regression."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
EXAMPLE = ROOT / "examples" / "enj_modifier_volume_zclip"
DEFAULT_BINARY = EXAMPLE / "build" / "pc-endjinn" / "enj_modifier_volume_zclip"
REFERENCE = Path(__file__).with_name("data") / "modifier_volume_visual_reference.json"
CAPTURE_FRAME = 180
GRID_WIDTH = 32
GRID_HEIGHT = 24

MODES = {
    "opaque_on": {},
    "opaque_off": {"ENJ_MODIFIER_CUBE_HIDDEN": "1"},
    "translucent_on": {"ENJ_MODIFIER_TRANSLUCENT": "1"},
    "translucent_off": {
        "ENJ_MODIFIER_TRANSLUCENT": "1",
        "ENJ_MODIFIER_CUBE_HIDDEN": "1",
    },
    "mixed_on": {"ENJ_MODIFIER_INSIDE_TRANSLUCENT": "1"},
    "mixed_off": {
        "ENJ_MODIFIER_INSIDE_TRANSLUCENT": "1",
        "ENJ_MODIFIER_CUBE_HIDDEN": "1",
    },
}


class Image:
    def __init__(self, width: int, height: int, pixels: bytes) -> None:
        self.width = width
        self.height = height
        self.pixels = pixels

    def pixel(self, x: int, y: int) -> tuple[int, int, int]:
        offset = (y * self.width + x) * 3
        return tuple(self.pixels[offset : offset + 3])  # type: ignore[return-value]


def read_ppm(path: Path) -> Image:
    with path.open("rb") as source:
        if source.readline().strip() != b"P6":
            raise AssertionError(f"{path}: expected binary PPM")
        dimensions = source.readline()
        while dimensions.startswith(b"#"):
            dimensions = source.readline()
        width, height = (int(value) for value in dimensions.split())
        if source.readline().strip() != b"255":
            raise AssertionError(f"{path}: expected 8-bit PPM")
        pixels = source.read()
    expected = width * height * 3
    if len(pixels) != expected:
        raise AssertionError(
            f"{path}: truncated pixel data ({len(pixels)} != {expected})"
        )
    return Image(width, height, pixels)


def capture(binary: Path, output: Path, frame: int,
            options: dict[str, str]) -> str:
    environment = os.environ.copy()
    for name in (
        "ENJ_MODIFIER_TRANSLUCENT",
        "ENJ_MODIFIER_INSIDE_TRANSLUCENT",
        "ENJ_MODIFIER_CUBE_HIDDEN",
        "ENJ_MODIFIER_EFFECT_DISABLED",
        "ENJ_TRANSLUCENT_SORT_DIAGNOSTICS",
    ):
        environment.pop(name, None)
    environment.update(
        {
            "ENJ_OFFSCREEN_CAPTURE": "1",
            "ENJ_SCREENSHOT_PATH": str(output),
            "ENJ_SCREENSHOT_FRAME": str(frame),
        }
    )
    environment.update(options)

    log_path = output.with_suffix(".log")
    with log_path.open("wb") as log:
        process = subprocess.Popen(
            [str(binary)], cwd=EXAMPLE, env=environment,
            stdout=log, stderr=subprocess.STDOUT,
        )
        deadline = time.monotonic() + 25.0
        expected_minimum = 1280 * 960 * 3 + len(b"P6\n1280 960\n255\n")
        try:
            while time.monotonic() < deadline:
                if output.exists() and output.stat().st_size >= expected_minimum:
                    break
                if process.poll() is not None:
                    raise AssertionError(
                        f"capture process exited early; see {log_path}"
                    )
                time.sleep(0.01)
            else:
                raise AssertionError(f"capture timed out; see {log_path}")
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=3.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=3.0)
    return log_path.read_text(errors="replace")


def fingerprint(image: Image) -> list[list[int]]:
    result: list[list[int]] = []
    for grid_y in range(GRID_HEIGHT):
        y0 = grid_y * image.height // GRID_HEIGHT
        y1 = (grid_y + 1) * image.height // GRID_HEIGHT
        for grid_x in range(GRID_WIDTH):
            x0 = grid_x * image.width // GRID_WIDTH
            x1 = (grid_x + 1) * image.width // GRID_WIDTH
            totals = [0, 0, 0]
            count = 0
            for y in range(y0, y1, 2):
                for x in range(x0, x1, 2):
                    red, green, blue = image.pixel(x, y)
                    totals[0] += red
                    totals[1] += green
                    totals[2] += blue
                    count += 1
            result.append([round(channel / count) for channel in totals])
    return result


def compare_fingerprint(name: str, actual: list[list[int]],
                        expected: list[list[int]]) -> None:
    if len(actual) != len(expected):
        raise AssertionError(f"{name}: reference grid size changed")
    differences = sorted(
        abs(a_channel - e_channel)
        for a_pixel, e_pixel in zip(actual, expected)
        for a_channel, e_channel in zip(a_pixel, e_pixel)
    )
    mean = sum(differences) / len(differences)
    percentile_95 = differences[int(len(differences) * 0.95)]
    if mean > 6.0 or percentile_95 > 18:
        raise AssertionError(
            f"{name}: visual fingerprint drifted "
            f"(mean={mean:.2f}, p95={percentile_95})"
        )
    print(f"  {name}: fingerprint mean={mean:.2f}, p95={percentile_95}")


def changed_fraction(a: Image, b: Image, *, y_limit: int | None = None,
                     threshold: int = 8) -> float:
    if (a.width, a.height) != (b.width, b.height):
        raise AssertionError("capture dimensions differ")
    changed = 0
    sampled = 0
    limit = a.height if y_limit is None else min(y_limit, a.height)
    for y in range(0, limit, 4):
        for x in range(0, a.width, 4):
            first = a.pixel(x, y)
            second = b.pixel(x, y)
            changed += max(abs(left - right) for left, right in zip(first, second)) > threshold
            sampled += 1
    return changed / sampled


def orange_mask_difference(a: Image, b: Image) -> float:
    changed = 0
    union = 0
    for y in range(0, min(a.height, b.height), 4):
        for x in range(0, min(a.width, b.width), 4):
            masks = []
            for image in (a, b):
                red, green, blue = image.pixel(x, y)
                masks.append(
                    red > 75 and red * 100 > green * 103 and
                    red * 100 > blue * 118
                )
            changed += masks[0] != masks[1]
            union += masks[0] or masks[1]
    return changed / max(union, 1)


def assert_mode_invariants(images: dict[str, Image], mixed_baseline: Image) -> None:
    render_height = images["opaque_on"].height * 3 // 4
    for mode in ("opaque", "translucent", "mixed"):
        difference = changed_fraction(
            images[f"{mode}_on"], images[f"{mode}_off"],
            y_limit=render_height,
        )
        if difference < 0.01:
            raise AssertionError(f"{mode}: B toggle did not change the volume visualization")

    if changed_fraction(images["opaque_off"], images["translucent_off"]) < 0.01:
        raise AssertionError("OP and TR receiver modes are visually indistinguishable")
    if changed_fraction(images["translucent_off"], images["mixed_off"]) < 0.01:
        raise AssertionError("TR and MIX receiver modes are visually indistinguishable")

    mixed = images["mixed_off"]
    floor_total = 0
    floor_changed = 0
    for y in range(mixed.height * 40 // 100, mixed.height * 64 // 100, 2):
        for x in range(0, mixed.width, 2):
            red, green, blue = mixed_baseline.pixel(x, y)
            if red > 45 and green > red + 5 and blue > red + 10:
                floor_total += 1
                current = mixed.pixel(x, y)
                floor_changed += max(
                    abs(left - right) for left, right in
                    zip((red, green, blue), current)
                ) > 2
    if floor_total < 1000:
        raise AssertionError("MIX floor probe did not find enough checker pixels")
    floor_drift = floor_changed / floor_total
    if floor_drift > 0.005:
        raise AssertionError(
            f"MIX modifier changed {floor_drift:.2%} of checker-floor probes"
        )

    orange_total = 0
    orange_changed = 0
    for y in range(0, mixed.height * 3 // 4, 2):
        for x in range(0, mixed.width, 2):
            red, green, blue = mixed_baseline.pixel(x, y)
            if red > 75 and red * 100 > green * 103 and red * 100 > blue * 118:
                orange_total += 1
                current = mixed.pixel(x, y)
                orange_changed += max(
                    abs(left - right) for left, right in
                    zip((red, green, blue), current)
                ) > 8
    if orange_total < 1000 or orange_changed / orange_total < 0.01:
        raise AssertionError("MIX modifier did not alter the orange receiver")

    print(f"  MIX checker-floor drift={floor_drift:.3%}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY)
    parser.add_argument("--update-reference", action="store_true")
    arguments = parser.parse_args()
    binary = arguments.binary.resolve()
    if not binary.is_file():
        raise SystemExit(f"missing pc-enDjinn example binary: {binary}")

    with tempfile.TemporaryDirectory(prefix="enj-modifier-visual-") as temporary:
        output_dir = Path(temporary)
        images: dict[str, Image] = {}
        diagnostic_log = ""
        for name, mode_options in MODES.items():
            options = dict(mode_options)
            if name == "translucent_on":
                options["ENJ_TRANSLUCENT_SORT_DIAGNOSTICS"] = "1"
            output = output_dir / f"{name}.ppm"
            log = capture(binary, output, CAPTURE_FRAME, options)
            if name == "translucent_on":
                diagnostic_log = log
            images[name] = read_ppm(output)

        if "pc-enDjinn: translucent-sort" not in diagnostic_log or \
                "cycle-breaks=" not in diagnostic_log:
            raise AssertionError("translucent-sort diagnostic counters were not emitted")

        fingerprints = {name: fingerprint(image) for name, image in images.items()}
        if arguments.update_reference:
            REFERENCE.parent.mkdir(parents=True, exist_ok=True)
            REFERENCE.write_text(json.dumps(
                {
                    "width": next(iter(images.values())).width,
                    "height": next(iter(images.values())).height,
                    "grid": [GRID_WIDTH, GRID_HEIGHT],
                    "capture_frame": CAPTURE_FRAME,
                    "fingerprints": fingerprints,
                },
                separators=(",", ":"),
            ) + "\n")
            print(f"updated {REFERENCE}")
        else:
            if not REFERENCE.is_file():
                raise AssertionError(
                    f"missing {REFERENCE}; run with --update-reference"
                )
            reference = json.loads(REFERENCE.read_text())
            if reference["grid"] != [GRID_WIDTH, GRID_HEIGHT] or \
                    reference["capture_frame"] != CAPTURE_FRAME:
                raise AssertionError("visual reference metadata is stale")
            for name in MODES:
                compare_fingerprint(
                    name, fingerprints[name], reference["fingerprints"][name]
                )

        baseline_path = output_dir / "mixed_disabled.ppm"
        capture(
            binary, baseline_path, CAPTURE_FRAME,
            {
                "ENJ_MODIFIER_INSIDE_TRANSLUCENT": "1",
                "ENJ_MODIFIER_CUBE_HIDDEN": "1",
                "ENJ_MODIFIER_EFFECT_DISABLED": "1",
            },
        )
        assert_mode_invariants(images, read_ppm(baseline_path))

        temporal_frames = (162, 168, 174, 180, 474, 480, 486, 492)
        temporal_images: dict[int, Image] = {}
        for frame in temporal_frames:
            if frame == CAPTURE_FRAME:
                temporal_images[frame] = images["translucent_off"]
                continue
            path = output_dir / f"temporal_{frame}.ppm"
            capture(
                binary, path, frame,
                {
                    "ENJ_MODIFIER_TRANSLUCENT": "1",
                    "ENJ_MODIFIER_CUBE_HIDDEN": "1",
                },
            )
            temporal_images[frame] = read_ppm(path)
        for group in ((162, 168, 174, 180), (474, 480, 486, 492)):
            for previous, current in zip(group, group[1:]):
                difference = orange_mask_difference(
                    temporal_images[previous], temporal_images[current]
                )
                if difference > 0.10:
                    raise AssertionError(
                        f"translucent continuity {previous}->{current} "
                        f"jumped by {difference:.2%}"
                    )
        print("modifier_volume_visual_regression: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
