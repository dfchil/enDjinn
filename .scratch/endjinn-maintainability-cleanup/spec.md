# enDjinn Maintainability Cleanup

Status: implemented and locally validated

## Problem Statement

enDjinn began as a small, deliberately handwritten Dreamcast project with a
clear personal style. Recent work added useful PC and Web development backends,
modifier-volume support, tests, capture tooling, and safety improvements, but it
also increased the amount of abstraction, duplication, commentary, backend
leakage, and generated-looking structure in the repository. The maintainer likes
the new capabilities but is concerned that their implementation quality and
maintenance cost could compromise the clarity of the original project.

The project boundary has also become less precise. Generic KOS/PVR emulation is
appropriate for enDjinn, while Model 1-specific painting and other Dream Driving
application behavior are not. Examples mix teaching code with capture
automation, host backends duplicate infrastructure, some public-looking APIs are
unused, and the Web backend does not yet match the PC backend in every modifier
rendering behavior.

The cleanup must improve correctness and maintainability without discarding
valuable recent work or rewriting the project into a generic engine framework.
It must retain the compact, understandable character of the original code and
make future contributions easier to judge against explicit boundaries.

## Solution

Clean up enDjinn incrementally as a Dreamcast-first engine and toolkit for the
maintainer's opinionated game-development workflow. Preserve useful PC, Web,
modifier-volume, test, and safety capabilities, but simplify their expression,
remove app-specific behavior, consolidate genuinely shared host infrastructure,
and document the project's intended boundaries.

Keep the public game-facing surface primarily in C and close to the existing
handwritten API. Use disciplined C++23 for private host-backend implementation.
Treat correctness repairs separately from structural cleanup, migrate known
consumers alongside intentional API changes, and verify each checkpoint through
the highest practical behavioral seam.

The result should provide reusable runtime facilities, asset generation, and a
coherent build process for making Dreamcast games, with PC and Web as development
backends rather than alternate product identities. Model 1-specific rendering
will remain modular but local to Dream Driving until a second real consumer
proves the need for a separate library.

## User Stories

1. As the enDjinn maintainer, I want the repository to retain its compact handwritten character, so that I can understand and confidently maintain it.
2. As the enDjinn maintainer, I want useful recent functionality preserved, so that cleanup does not throw away working modifier, PC, Web, capture, or safety improvements.
3. As the enDjinn maintainer, I want code judged by clarity and necessity rather than authorship, so that valuable generated contributions survive while low-quality cruft does not.
4. As a Dreamcast game author, I want enDjinn to remain Dreamcast-first, so that its design follows the platform I am actually targeting.
5. As a Dreamcast game author, I want PC and Web backends for development, so that I can iterate and inspect behavior without requiring hardware for every change.
6. As the enDjinn maintainer, I want a clear ownership boundary, so that game-specific and arcade-hardware-specific features do not accumulate in the engine.
7. As the Dream Driving maintainer, I want Model 1 painting code to remain app-local and internally modular, so that it can evolve without coupling enDjinn to one game.
8. As a future Model 1 port author, I want app-local modules to be extractable later, so that a proven second consumer can justify a dedicated library.
9. As an enDjinn user, I want the existing public C API preserved where it is already sound, so that cleanup does not cause gratuitous migration work.
10. As the enDjinn maintainer, I want limited intentional API breakage to be allowed, so that unused or poorly placed interfaces do not become permanent baggage.
11. As a known downstream consumer, I want API migrations performed in the same change as the break, so that verified projects do not remain temporarily broken.
12. As the enDjinn maintainer, I want unused compatibility aliases removed instead of retained indefinitely, so that the public surface remains honest and small.
13. As a contributor, I want portable behavior separated from host-only facilities and backend diagnostics, so that ownership and portability are obvious.
14. As a contributor, I want shared PC and Web implementation placed in a private host-common area, so that duplicated behavior is consolidated without exposing a new public API.
15. As a contributor, I want large host renderer files split by coherent responsibility, so that changes can be reviewed and tested locally.
16. As a contributor, I want file size treated as a warning rather than a rigid rule, so that decomposition follows real responsibilities instead of arbitrary line counts.
17. As a host-backend maintainer, I want private implementation written in disciplined C++23, so that modern language facilities can reduce lifetime and state-management errors.
18. As a Dreamcast-facing API user, I want public headers and examples to remain C-oriented, so that host implementation choices do not leak into game code.
19. As a PC backend user, I want screenshot requests to own their paths and capture the intended frame safely, so that delayed capture cannot use invalid pointers or swapchain state.
20. As a PC backend user, I want a single explicit next-frame capture request with rejection when one is already pending, so that capture behavior is deterministic.
21. As a Web backend user, I want alpha-cutout rendering to match the shared primitive state, so that punch-through content behaves consistently across host backends.
22. As a Web backend user, I want batching to account for alpha-cutout state, so that primitives with different material behavior are not merged incorrectly.
23. As a modifier-volume example user, I want PC and Web results to reflect Dreamcast concepts closely enough for development, so that examples teach transferable behavior.
24. As a learner, I want a small planar modifier example named `enj_flat_modifiers`, so that the introductory example communicates its intentionally shallow geometry.
25. As a learner, I want a closed-volume example named `enj_deep_modifiers`, so that depth, clipping, transparent receivers, and complete 3D modifier geometry are demonstrated clearly.
26. As a learner, I want `flat` and `deep` presented as enDjinn teaching terms, so that I do not mistake them for official KOS API categories.
27. As a learner, I want example source focused on the concept being taught, so that capture automation and regression controls do not obscure it.
28. As the enDjinn maintainer, I want capture and test automation outside example programs, so that examples remain literal, readable demonstrations.
29. As an existing example user, I want the original non-modifier examples preserved with minimal stylistic cleanup, so that familiar teaching material remains recognizable.
30. As the enDjinn maintainer, I want comments limited to non-obvious invariants and hardware constraints, so that source does not read like generated narration or a changelog.
31. As a contributor, I want merge archaeology and self-justifying comments removed, so that current code explains current behavior rather than its editing history.
32. As a contributor, I want correctness changes separated from structural changes, so that regressions are easier to identify and reviews remain meaningful.
33. As a contributor, I want behavior-preserving cleanup commits, so that refactoring can be validated independently from output changes.
34. As a reviewer, I want golden output changes justified by a specific correctness repair, so that visual baselines cannot drift casually.
35. As the enDjinn maintainer, I want fast headless checks available through the default test command, so that routine validation stays cheap.
36. As a renderer maintainer, I want explicit PC/Vulkan and browser/WebGL integration checks, so that platform behavior is tested at the rendering boundary rather than inferred from compilation.
37. As a Dreamcast maintainer, I want the KOS ABI-subset smoke test retained and expanded only as needed, so that host compatibility remains useful without pretending to implement all of KOS.
38. As the enDjinn maintainer, I want the existing tests reorganized and rewritten where necessary rather than discarded, so that their useful coverage survives cleanup.
39. As a modifier-volume maintainer, I want realistic performance measurements and a documented practical budget, so that examples do not imply an unusable maximum.
40. As a modifier-volume maintainer, I want a conservative hard limit and clear failure behavior, so that pathological geometry cannot silently overwhelm host rendering.
41. As the enDjinn maintainer, I want spatial acceleration deferred until measurement proves it necessary, so that speculative complexity is not added prematurely.
42. As a build-system user, I want strong conventional defaults with a small set of intentional overrides, so that common projects require little boilerplate.
43. As a build-system maintainer, I want unused discovery rules and speculative configuration removed, so that build behavior is predictable.
44. As a game author, I want enDjinn to own engine-specific asset contracts and lightweight recipes, so that assets fit naturally into the game build.
45. As a game author, I want complex conversion delegated to appropriate external tools, so that enDjinn does not become a general asset-processing suite.
46. As the enDjinn maintainer, I want old profiling scripts classified as reusable, app-specific, or dead, so that the repository contains only supportable tooling.
47. As the Dream Driving maintainer, I want active game-specific profiler workflows moved to the app, so that ownership follows their real consumer.
48. As an enDjinn user, I want concise root and usage documentation, so that the project identity and normal integration path are easy to discover.
49. As a backend user, I want backend-specific installation requirements and limitations documented near each backend, so that platform setup remains actionable.
50. As a prospective contributor, I want a concise support matrix, so that portable, host-only, and diagnostic features are distinguishable at a glance.
51. As a contributor, I want the SH4ZAM integration retained only if it can be simplified and exercised by a real consumer, so that optional integration does not become abandoned scaffolding.
52. As the enDjinn maintainer, I want the standalone enDjinn checkout to remain authoritative, so that cleanup does not fork silently inside an integration checkout.
53. As the Dream Driving maintainer, I want its enDjinn pointer updated only at verified checkpoints, so that the application remains buildable throughout cleanup.
54. As a reviewer, I want cleanup delivered in a staged sequence, so that correctness, ownership, architecture, examples, tests, and documentation can be assessed separately.
55. As a future maintainer, I want concise migration notes for intentional breaks, so that downstream updates are clear without preserving temporary documentation forever.

## Implementation Decisions

- Define enDjinn as a Dreamcast-first engine and toolkit for an opinionated game-development workflow. It includes reusable runtime facilities, engine-specific asset generation, and build integration. PC and Web exist primarily as development backends.
- Keep generic KOS/PVR semantics and broadly useful engine facilities in enDjinn. Keep game-specific, arcade-hardware-specific, and Model 1-specific behavior in the consuming application.
- Keep Model 1 rendering app-local but internally modular. Do not extract a separate library until a second real consumer establishes stable shared requirements.
- Preserve sound handwritten public APIs by default. Allow focused breakage when it removes a misplaced or unused interface, and migrate all known consumers in the same logical change.
- Do not add compatibility aliases for APIs with no known consumers merely to avoid a nominal break.
- Classify features as portable behavior, generic host-development behavior, or isolated backend-specific diagnostics. Public APIs must not accidentally expose the third category as portable engine functionality.
- Treat the April 2026 `f7a3984` revision as a stylistic reference point, not a rollback target. Retain useful behavior introduced later while rewriting unnecessarily complex expression.
- Evaluate suspected generated-code cruft by concrete qualities: needless abstraction, duplicated logic, speculative generality, excessive narration, inconsistent terminology, unclear ownership, and avoidable indirection.
- Keep comments that document non-obvious invariants, PVR/KOS constraints, synchronization, precision, ownership, or lifetime. Remove editing history, self-justification, repeated prose, and commentary that only restates code.
- Introduce a private host-common implementation area for behavior genuinely shared by PC and Web. It is not a public include surface for applications.
- Split large host files along meaningful responsibilities such as command/state translation, resource management, modifier evaluation, frame lifecycle, capture, and backend submission. Roughly 500–800 lines is a useful review signal, not a mandatory limit.
- Use disciplined C++23 in private host code: explicit ownership, scoped resource management, narrow interfaces, value types where practical, and no unnecessary template or framework machinery.
- Keep the game-facing API and examples in C unless a specific public feature demonstrably benefits from another boundary.
- Replace unsafe immediate/late screenshot assumptions with an explicit next-frame capture extension. The extension owns the requested path, allows one pending request, reports rejection, and captures from a transfer-capable image in a valid layout.
- Correct Web alpha-cutout parity by carrying the shared primitive state through batching and rendering instead of inferring cutout behavior only from list type.
- Rename the planar modifier example to `enj_flat_modifiers` and make its geometry genuinely planar and screen-oriented.
- Rename the closed-volume example to `enj_deep_modifiers` and use it to demonstrate complete six-sided volume geometry, depth interaction, clipping, opaque/translucent visualization, and transparent receiver behavior.
- Describe `flat` and `deep` as enDjinn pedagogical terms. KOS exposes modifier-volume primitives and distinguishes list/material modes such as opaque versus translucent and inclusion versus exclusion; it does not define separate shallow and deep APIs.
- Move capture sequencing, toggle-matrix automation, and visual regression controls out of example source into test or tooling layers.
- Preserve the eight original non-modifier examples, applying only targeted cleanup needed for consistent build and presentation.
- Remove the unused raw Web rendering extension from the public API unless a real consumer is identified during implementation.
- Move PC-only diagnostics out of KOS compatibility headers and expose them through an explicit backend extension when they remain useful.
- Consolidate common PC/Web source under neutral host naming instead of compiling PC-named implementation as part of Web.
- Keep build defaults strong and conventional. Retain overrides only when they support real project variation; remove unused auto-discovery and speculative configuration.
- Let enDjinn define asset formats, generated-header contracts, and small build recipes needed by its runtime. Delegate substantial media conversion to focused external tools.
- Inspect each legacy profiling tool. Move active Dream Driving-specific workflows to that application, parameterize genuinely reusable tools, and delete dead tools only after confirming they have no consumer.
- Benchmark modifier evaluation before redesigning it. Publish a realistic practical triangle/volume budget and enforce a conservative safety limit. Defer spatial indexing or other acceleration until measurements justify it.
- Keep the KOS compatibility test explicitly framed as an ABI-subset smoke test. Expand coverage incrementally as supported surface area grows.
- Retain SH4ZAM integration only if cleanup produces a small understandable integration exercised by a real consumer; otherwise remove it as unused optional scaffolding.
- Keep documentation layered: a concise project identity and minimal build path at the root, one durable usage guide, backend-local setup and limitations, a short support matrix, and examples that teach by code.
- Perform work on a dedicated `cleanup/host-backends` branch in the authoritative standalone checkout. Update the Dream Driving integration pointer only after relevant checks pass.
- Deliver cleanup in this order: correctness repairs; API/backend leakage and dead surface removal; host-common extraction; large-file decomposition; examples and test simplification; documentation; separately approved deeper renderer redesign.
- Keep structural commits behavior-preserving. Isolate output changes in evidence-backed correctness commits. Update golden references only when the intended correction is documented.
- Provide a concise migration note for intentional public breaks, then fold enduring guidance into the usage documentation rather than accumulating historical notes.

## Testing Decisions

- Test externally observable behavior instead of private structure. Refactoring must not require tests to know how host-common code or renderer files are divided.
- Use the existing fast `make check` path as the routine high-level seam for core behavior and API safety.
- Retain and improve the existing core-safety, resource-safety, public-C++-consumption, texture-shim, translucent-sort, and modifier visual-regression coverage.
- Treat the core and resource safety tests as contract checks for lifecycle, invalid input, allocation failure, and public state transitions.
- Treat the public C++ consumption test as a narrow compile/link guard proving that the C-facing headers remain usable by host C++ implementation and consumers.
- Treat texture-shim and translucent-sort tests as behavioral seams for compatibility translation and order-dependent host rendering logic.
- Exercise PC modifier rendering through an explicit headless Vulkan integration target that renders representative scenes and captures a completed frame through the public backend extension.
- Exercise Web modifier rendering through a browser/WebGL integration target that validates actual output, not merely successful compilation.
- Use the same conceptual modifier scenes across PC and Web: opaque outside/inside behavior, translucent volume visualization, transparent receivers, alpha cutout, clipping, complete closed geometry, and all supported toggle combinations.
- Keep capture orchestration and toggle-matrix traversal in the test harness rather than in example applications.
- Verify screenshot requests for owned-path lifetime, single-pending-request rejection, valid transfer usage, correct image layout transition, and delivery of the intended frame.
- Add a focused Web regression proving that alpha-cutout state participates in batch identity and produces the same visible discard behavior as the PC path.
- Keep visual references small and reviewable. Replace opaque one-line image dumps with a representation and update process that makes intentional output changes inspectable.
- Require a written correctness reason whenever a golden reference changes.
- Keep the Dreamcast/KOS ABI-subset smoke test as the highest practical seam for the compatibility surface, without claiming full KOS compatibility.
- Build the representative Dreamcast examples after example renames and build-system cleanup.
- Build Dream Driving for PC and Web at integration-pointer checkpoints. Include its Dreamcast build when the relevant toolchain is available.
- Benchmark modifier workloads using representative scene sizes before setting documented budgets or safety limits.
- Keep the default test command fast and headless. Put Vulkan runtime, browser runtime, and hardware/toolchain-dependent checks behind clearly named explicit targets.
- Do not add a new seam solely to test an implementation detail. Prefer one renderer-level output seam per backend plus the existing portable unit/contract seam.

## Out of Scope

- Turning enDjinn into a universal engine, a general KOS replacement, or a compatibility layer for all Dreamcast software.
- Moving Model 1 painting, racing-game rendering, or other Dream Driving-specific behavior back into enDjinn.
- Extracting a standalone Model 1 library before a second real application requires it.
- Reverting wholesale to the pre-host-backend codebase or deleting useful recent modifier, PC, Web, safety, and capture capabilities.
- Preserving every recent abstraction or public symbol solely for source compatibility when it has no known consumer.
- Rewriting the public API into C++.
- Introducing a broad host framework, plugin architecture, dependency-injection system, or speculative backend abstraction.
- Implementing spatial acceleration for modifier volumes before benchmarks demonstrate the need.
- Guaranteeing pixel-identical output across Dreamcast PVR, Vulkan, and WebGL where the underlying platforms differ; documented semantic parity is the goal.
- Building a general-purpose asset conversion suite inside enDjinn.
- Performing a deep renderer redesign as part of the initial cleanup sequence.
- Automatically deleting legacy scripts or optional integrations without first establishing whether they have a real consumer.

## Further Notes

- The standalone enDjinn repository is authoritative. The copy used by Dream Driving is an integration checkout and should follow verified standalone commits rather than becoming an independent source of truth.
- Existing audits identified two correctness priorities: unsafe late Vulkan capture state/path ownership and missing Web alpha-cutout participation in batching/rendering.
- The host implementations currently contain substantial duplication and oversized translation units. Extraction should follow proven common behavior, not begin with a theoretical interface hierarchy.
- Modifier rendering currently evaluates all relevant modifier triangles per participating fragment. The advertised maximum must not be confused with a practical budget; benchmark before choosing limits or optimization work.
- The current deep modifier example combines teaching, interactive controls, and automated capture concerns. Its cleanup is a representative test of the intended example policy.
- The current regression assets and Web compile-only validation need improvement so visual correctness is reviewable and runtime parity is exercised.
- The repository already contains literal examples rather than a broad authored test suite. Existing added tests should be kept where they protect behavior, but their presentation should be brought in line with the project's style.
- The established testing seam is accepted: portable contract tests through the fast test target, one renderer-level visual seam for PC, one browser-level visual seam for Web, the KOS ABI-subset smoke test, and downstream Dream Driving builds at integration checkpoints.
- The local specification is the source of truth for this cleanup effort. It may later be split into implementation tickets without changing its approved boundaries.
