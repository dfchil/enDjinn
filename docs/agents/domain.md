# Domain documentation

This repository uses a single-context documentation layout.

## Before exploring

Read these sources when they exist:

- `CONTEXT.md` at the repository root
- relevant architectural decisions under `docs/adr/`

Missing files are not errors. Proceed silently instead of creating placeholder
documentation. Add domain documentation only when terminology or architectural
decisions have actually been resolved.

## Current routing areas

Use these areas to locate relevant code and keep documentation focused:

- `engine-core`: public C API, runtime state, rendering, input, sound, fonts,
  textures, and Dreamcast behavior
- `host-backends`: shared host behavior and the PC/Vulkan and Web/WebGL
  development backends
- `build-and-assets`: Make integration, feature switches, asset generation,
  converters, and profiling tools
- `examples-and-tests`: teaching examples, automated tests, visual regression,
  and backend verification

These are areas within one enDjinn context, not separate packages or bounded
contexts.

## Vocabulary

Use the terminology defined in `CONTEXT.md`. Avoid introducing synonyms for
terms the glossary defines explicitly. If a needed concept is absent, either
reconsider the terminology or record it as a domain-modeling gap.

## Architectural decisions

Surface conflicts with existing ADRs explicitly rather than silently
overriding them.
