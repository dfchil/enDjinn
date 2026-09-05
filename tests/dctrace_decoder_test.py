#!/usr/bin/env python3
import importlib.util
import io
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "dctrace", ROOT / "profilers" / "dctrace.py")
dctrace = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(dctrace)


def uleb128(value):
    encoded = bytearray()
    while True:
        byte = value & 0x7f
        value >>= 7
        if value:
            byte |= 0x80
        encoded.append(byte)
        if not value:
            return encoded


def record(thread_id, address, entry, delta_time, delta_e0=0, delta_e1=0):
    compressed = (address - dctrace.BASE_ADDRESS) >> 2
    header = ((dctrace.ENTRY_FLAG if entry else 0) |
              (thread_id << 22) | compressed)
    return (struct.pack("<I", header) + uleb128(delta_time) +
            uleb128(delta_e0) + uleb128(delta_e1))


def main():
    parent = dctrace.BASE_ADDRESS + 0x100
    child = dctrace.BASE_ADDRESS + 0x200
    other_thread = dctrace.BASE_ADDRESS + 0x300
    trace = b"".join([
        record(1, parent, True, 10, 1, 2),
        record(2, other_thread, True, 20, 3, 4),
        record(1, child, True, 2, 1, 1),
        record(2, other_thread, False, 5, 2, 3),
        record(1, child, False, 3, 2, 2),
        record(1, parent, False, 4, 3, 4),
    ])

    totals = dctrace.decode_trace(
        io.BytesIO(trace), len(trace), hex, show_progress=False)

    assert totals == (25 * 80, 7, 9)
    assert dctrace.functions[parent].total_time == 9 * 80
    assert dctrace.functions[child].total_time == 3 * 80
    assert dctrace.functions[other_thread].total_time == 5 * 80
    assert dctrace.child_calls[parent][child].times_called == 1
    assert dctrace.child_calls[parent][child].total_cycles == 3 * 80
    assert other_thread not in dctrace.child_calls[parent]
    print("dctrace_decoder_test: ok")


if __name__ == "__main__":
    main()
