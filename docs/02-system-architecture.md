# System architecture

## 1. Technology baseline

- C++20
- Qt 6.8 LTS-compatible APIs
- QML and Qt Quick Controls for the application shell
- Native Qt text input and scene graph rendering
- `cmark-gfm` for CommonMark/GFM parsing
- CMake 3.25 or later with CMake Presets
- Catch2 or Qt Test for unit and integration tests

Qt WebEngine, Electron, Tauri, and embedded browser controls are excluded from
the core application.

## 2. Architectural shape

The codebase follows ports-and-adapters boundaries. UI code may call application
commands and query view models, but it may not directly access the filesystem or
mutate the Markdown syntax tree.

```text
QML shell and native editor items
              |
        Application layer
     commands, queries, use cases
              |
   Domain model and document engine
              |
   -------------------------------
   | parser | files | export | OS |
   -------------------------------
```

## 3. Components

### `app`

Owns process startup, dependency construction, command-line handling, single
instance behavior, translations, crash recovery startup, and application life
cycle. It contains no document algorithms.

### `ui`

Contains QML views, C++ view models, commands, dialogs, focus management, theme
tokens, accessibility metadata, and platform window integration.

### `editor`

Provides two native editor surfaces:

- `SourceEditor`: a virtualized plain-text editor with Markdown highlighting.
- `BlockEditor`: an AST-backed sequence of editable block items.

Both implement the same `IEditorSurface` contract for selection, commands,
history, clipboard, and navigation.

### `document`

Owns `DocumentSession`, the text buffer, parsed syntax tree, source map, document
metadata, dirty state, undo transactions, diagnostics, and file revision.

Only `DocumentSession` may commit mutations to an open document. UI operations
are expressed as commands such as `SplitParagraph`, `ToggleEmphasis`, or
`MoveBlocks`.

### `markdown`

Wraps cmark-gfm behind stable project-owned interfaces. It converts parser nodes
into immutable project AST nodes with stable IDs, source spans, attributes, and
extension data. It also serializes edited nodes and creates the render tree.

Project code must not expose `cmark_node*` outside this module.

### `render`

Transforms the Markdown AST into native layout objects. Text uses Qt shaping and
font fallback. Images use `QImageReader`. Code, tables, equations, and diagrams
are specialized render nodes. Reading mode virtualizes blocks outside the
viewport.

### `workspace`

Owns file discovery, ignore rules, outline indexing, content search, resource
resolution, recent files, and `QFileSystemWatcher` integration. Indexing runs in
a bounded worker pool and never blocks the GUI thread.

### `storage`

Implements byte decoding, newline detection, advisory file identity, atomic
write, recovery journals, permissions, and external-change detection.

### `export`

Consumes the project render tree rather than editor widgets. HTML has a dedicated
safe serializer. PDF and printing share a paginated native layout pipeline.

### `platform`

Contains the smallest possible adapters for native menus, title bars, file
associations, sandbox/bookmark access on macOS, notifications, and package data
locations.

## 4. Document pipeline

```text
bytes -> decoded text -> text buffer -> cmark parse -> project AST
   -> source map -> editor blocks -> native render tree -> screen/export
```

After a local edit, only the affected block range is serialized and reparsed.
The parser runs on an immutable text snapshot. Results carry a document revision;
stale results are discarded rather than applied to newer text.

Full reparse is permitted after structural ambiguity, extension setting changes,
or external file replacement. Parsing never mutates UI-owned objects.

## 5. Threading model

- GUI thread: QML, input, selection, visible-block layout, command dispatch.
- Parser workers: parsing and AST reconciliation on immutable snapshots.
- I/O workers: file reads, indexing, hashing, resource loading.
- Render workers: syntax tokenization, diagram and equation preparation where
  the underlying library permits it.

All cross-thread results are immutable value objects. QObject ownership remains
on its creating thread. Cancellation uses `std::stop_token` or a project-owned
cancellation token.

## 6. Persistence and recovery

Normal save performs:

1. Compare current on-disk identity/revision with the opened revision.
2. Serialize the current document using the chosen encoding and newline style.
3. Write and flush a sibling temporary file where the platform permits.
4. Preserve applicable permissions and atomically replace the destination.
5. Update document revision and remove the corresponding recovery journal.

Recovery journals live in the platform application-data directory, not next to
the user's file. They contain the base file identity and compact edit operations.
They are periodically flushed and removed after a confirmed save or discard.

## 7. Dependency direction

```text
app -> ui -> application -> document -> markdown
                     |        |           |
                     v        v           v
                  ports <- adapters: storage/render/workspace/export/platform
```

Lower layers must not import QML types. Domain and parser tests must run without
creating a GUI application.

## 8. Repository layout

```text
app/
src/application/
src/document/
src/markdown/
src/editor/
src/render/
src/workspace/
src/storage/
src/export/
src/platform/
qml/
resources/
themes/
tests/unit/
tests/integration/
tests/conformance/
packaging/
third_party/
```

