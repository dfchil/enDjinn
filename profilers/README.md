# Dreamcast profiling

enDjinn retains two target-side profiling mechanisms because both are reusable
across Dreamcast applications:

- `ENJ_DCTRACE=1` enables function entry/exit instrumentation and writes
  `/pc/trace.bin`. Decode it with `dctrace.py` and render the resulting
  `graph.dot` with Graphviz.
- `ENJ_DCPROF=1` enables the sampling profiler and writes `/pc/gmon.out` for
  `sh-elf-gprof`.

Both output paths require the application to run through dcload with a writable
`/pc` mount. Launch commands, network addresses, selected game modes, output
directories, and compiler exclusions belong to the consuming application's
scripts; they are deliberately not encoded here.

Example analysis commands:

```sh
python3 /path/to/enDjinn/profilers/dctrace.py \
  -t cdrom/trace.bin bin/my-game.elf
dot -Tsvg graph.dot -o graph.svg

sh-elf-gprof bin/my-game.elf cdrom/gmon.out > profile.txt
```

`KOS_ADDR2LINE` can override the decoder's `sh-elf-addr2line` executable.

The files under `dcprof/` are derived from
[Dreamcast-Projects/dcprofiler](https://github.com/Dreamcast-Projects/dcprofiler)
and retain their upstream license and attribution.
