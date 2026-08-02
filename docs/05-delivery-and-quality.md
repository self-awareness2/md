# Delivery and quality plan

## 1. Delivery stages

### Stage 0: foundation

- CMake presets, dependency locking, CI, formatting, static analysis.
- Application shell, theme tokens, logging, settings, translations.
- Cross-platform packaging smoke builds.

Exit: empty signed/debuggable application launches on all target systems and the
unit-test suite runs in CI.

### Stage 1: reader MVP

- Safe file open, decoding, cmark-gfm wrapper, project AST.
- Native reading renderer for core CommonMark and GFM.
- Outline, local resource resolution, light/dark themes.
- File watcher and read-only large-document behavior.

Exit: CommonMark/GFM fixture documents render correctly without WebEngine.

### Stage 2: source editor

- Native text buffer, syntax highlighting, selection, clipboard, undo/redo.
- Atomic save, recovery journal, external-change handling.
- Find/replace and source/reading mode synchronization.

Exit: source mode is safe for daily editing and passes recovery tests.

### Stage 3: block editor

- Paragraphs, headings, lists, quotes, code, inline styles, links, images.
- Stable-ID reconciliation, source-preserving serialization, IME handling.
- Tables, tasks, footnotes, formulas, diagrams, and raw HTML fallback.

Exit: supported syntax round-trips through block editing with golden tests.

### Stage 4: workspace and export

- Workspace tree, indexed search, resource tools.
- HTML/PDF/print pipeline and export themes.
- Preferences, shortcuts, localization, accessibility pass.

Exit: feature-complete beta packages are available on all target systems.

### Stage 5: stabilization and 1.0

- Performance budgets, fuzzing, crash recovery drills, security review.
- Installer signing/notarization, update metadata, documentation.

Exit: all requirements in the 1.0 acceptance definition are satisfied.

## 2. Testing strategy

| Layer | Coverage |
| --- | --- |
| Unit | Text offsets, commands, serializer, source maps, encoding, path rules |
| Property | Parse/serialize invariants, command apply/invert, random Unicode edits |
| Conformance | CommonMark examples and GFM extension suites |
| Golden | Markdown input, AST, rendered snapshot, round-trip output |
| Integration | Open/edit/save/reload, external changes, crash recovery, exports |
| UI | Keyboard flows, focus, accessibility tree, screenshots, theme contrast |
| Fuzz | Parser wrapper, HTML sanitizer, clipboard conversion, file decoder |
| Performance | Startup, parse, layout, typing latency, memory, huge files |

Tests must include UTF-8/UTF-16, CJK input, emoji sequences, combining marks,
right-to-left text, very long lines, malformed Markdown, missing resources, and
read-only files.

## 3. Continuous integration

CI builds with current supported MSVC, Apple Clang, and GCC/Clang. Pull requests
run formatting checks, warnings-as-errors builds, unit tests, conformance tests,
and a minimal QML launch test. Scheduled builds add sanitizers, fuzz smoke runs,
packaging, and performance comparisons.

Dependencies are pinned and accompanied by license metadata. Release artifacts
include dependency notices and checksums.

## 4. Logging and privacy

Logs contain component, severity, timestamp, and an opaque session identifier.
Document contents, paths, clipboard contents, and rendered text are excluded by
default. Diagnostic bundles require explicit user action and show included data
before creation. No telemetry or network error reporting is enabled by default.

## 5. Risks

| Risk | Mitigation |
| --- | --- |
| AST/source positions drift | Immutable revisions, golden tests, stable-ID reconciliation |
| Block editing normalizes source | Reuse clean source slices; expose normalization operations |
| QML delegate churn harms typing | Virtualization, pooling, active-block native item |
| Complex tables and bidi selection | Dedicated interaction tests and staged delivery |
| Math/diagram scope expands | Adapter boundary and source fallback are mandatory |
| Platform text input differs | Per-platform IME and accessibility test matrix |
| cmark node API leaks | Project-owned AST is the only public parser output |

## 6. Development gate

Before Stage 0 product code begins, the team must review and accept:

- The supported Markdown profile and explicit 1.0 non-goals.
- Source preservation and normalization behavior.
- The native block editor command and selection models.
- Equation and diagram adapter choices.
- Packaging targets and minimum OS versions.

Any material change after acceptance requires an Architecture Decision Record.

