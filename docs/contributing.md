# Contributing

[Documentation](README.md)

enDjinn is a Dreamcast-first engine and toolkit for one opinionated way of
making games. These rules come from the
[maintainability cleanup](../.scratch/endjinn-maintainability-cleanup/spec.md)
that set its current boundaries.

## What belongs in enDjinn

- Generic KOS and PVR behaviour, reusable runtime facilities, enDjinn's own
  asset formats and the small build recipes that produce them.
- PC and Web exist as development backends, not as separate products.
  Generic host-development behaviour is shared in the private
  `backends/host-common/`; backend-only diagnostics stay in their backend and
  are not exposed as portable API (for example `<pc_endjinn/capture.h>`).
- Game-specific, arcade-hardware-specific and Model 1-specific behaviour stays
  in the application that needs it, until a second real consumer exists.
- Substantial media conversion belongs in external tools, not in enDjinn.

## Code

- The public API and the examples are C. Private host-backend code is C++23
  with explicit ownership and narrow interfaces, without framework machinery.
- Keep sound existing public APIs. An intentional break is allowed when it
  removes a misplaced or unused interface; migrate every known consumer in the
  same change and write a short migration note. Do not keep aliases for APIs
  with no known consumer.
- Comments explain non-obvious invariants, hardware constraints,
  synchronisation, precision, ownership and lifetime. They do not narrate the
  code or its editing history.
- Split large files along real responsibilities. Several hundred lines is a
  signal to look, not a limit.
- Keep correctness fixes and behaviour-preserving restructuring in separate
  commits.
- Examples teach one feature each. Capture and regression controls belong in
  `tests/`, not in example source.

## Checks

Before a change, run:

```sh
make -C tests check
make -C examples ENJ_TARGET=pc-endjinn
```

and the explicit renderer targets in the [test guide](../tests/README.md) when
rendering changes. A change to a golden reference needs a written correctness
reason. After updating KOS, regenerate the
[KOS ABI contract](../backends/pc-endjinn/README.md#kos-abi-contract).

## Documentation

Every document is listed in the [documentation index](README.md) with one
line saying what it is for. Topic documents live in `docs/`; backend setup
and limitations stay next to each backend. Each document starts with a link
back to the index and covers one subject. Describe what the code does today,
and check build variables against `cfg.mk`, `defines.mk` and `base_link.mk`.

## Issues and agents

Issues and specifications are local Markdown under `.scratch/`; the
conventions are in [the issue tracker guide](agents/issue-tracker.md). Agent
instructions start at [`AGENTS.md`](../AGENTS.md), and
[domain documentation](agents/domain.md) lists the code areas.
