# Native AST block editor

## 1. Design principle

The Markdown source remains authoritative for storage, while the project AST is
authoritative for structural editing. Block mode never treats formatted HTML as
an intermediate representation.

The editor is a virtualized list of block controllers. Each controller owns a
stable node ID, source span, layout item, local selection mapping, and commands
valid for its node type.

## 2. Node model

```cpp
using NodeId = std::uint64_t;

struct SourceSpan {
    qsizetype beginUtf16;
    qsizetype endUtf16;
};

struct AstNode {
    NodeId id;
    NodeKind kind;
    SourceSpan source;
    AttributeMap attributes;
    std::vector<AstNode> children;
};
```

The production representation may use compact immutable storage rather than the
illustrative recursive type. IDs are retained during incremental reconciliation
when node kind, source neighborhood, and content identity match.

## 3. Block types

The first implementation must define editing behavior for:

- Paragraph and heading
- Block quote
- Ordered, unordered, and task list item
- Fenced and indented code block
- Table, row, and cell
- Thematic break
- Image/media block
- Display equation
- Diagram block
- Raw HTML block
- Footnote definition

Unknown extension nodes remain source-editable and render as a non-destructive
fallback block. The editor must never delete syntax it does not understand.

## 4. Inline model

Paragraph-like blocks keep a plain-text input buffer plus immutable style spans.
Styles represent semantic marks: emphasis, strong, strike, code, link, image,
math, and footnote reference. Cursor positions are expressed in UTF-16 for Qt
integration and translated to byte offsets only at parser boundaries.

IME pre-edit text is transient and must not enter the command history or trigger
structural Markdown conversions until committed.

## 5. Editing state machine

```text
Idle -> Selecting -> Idle
Idle -> Composing IME -> Commit/Cancel -> Idle
Idle -> Applying command -> Reconcile AST -> Restore selection -> Idle
Idle -> Dragging blocks -> Commit move -> Reconcile AST -> Idle
Idle -> Conflict -> Reload/Merge/Keep local -> Idle
```

No parse result may replace the document while an IME composition is active.
Structural shortcuts are evaluated after committed input and only at defined
positions, such as `# ` at the beginning of a paragraph.

## 6. Command protocol

All mutations are commands with `apply`, `invert`, affected source range, and
selection-before/after data. Related typing operations coalesce into a single
undo transaction until a cursor move, structural edit, paste, or timeout.

Required command families include:

- Insert/delete text and split/join block
- Change block type and heading level
- Toggle inline mark
- Indent/outdent and reorder list item
- Insert/delete/move table row or column
- Toggle task state
- Insert or relink resource
- Move, duplicate, or delete blocks

Undo operates on committed commands. Parser reconciliation is derived state and
does not create a second undo entry.

## 7. Source preservation

Each AST node retains its source slice and a dirty flag. Serialization follows:

1. Reuse the original source slice for unchanged nodes.
2. Regenerate only dirty nodes using the configured style profile.
3. Preserve blank-line trivia attached to neighboring nodes.
4. Reparse the changed envelope and reconcile stable IDs.

This preserves users' delimiter preferences, reference links, list markers, and
spacing outside edited structures. Operations that necessarily normalize syntax
must be documented and covered by golden round-trip tests.

## 8. Selection and navigation

Selection is stored as two logical positions: node ID, text offset, and affinity.
Cross-block selections are supported. After reconciliation, positions are mapped
using retained IDs and edit deltas; the nearest surviving position is used as a
fallback.

Keyboard navigation follows platform conventions, supports bidirectional text,
grapheme clusters, word movement, page movement, and accessible focus traversal.

## 9. Clipboard

Copy provides `text/plain`, Markdown, and safe HTML MIME forms. Paste preference
is Markdown, then plain text; HTML is converted through a restricted structural
converter. File and image clipboard data is delegated to the resource service.

## 10. Rendering special content

- Tables use virtualized rows for large bodies and keyboard cell navigation.
- Equations parse to an internal box layout or a vetted native library adapter.
- Mermaid/PlantUML diagrams render to sanitized SVG through optional adapters;
  failure displays source plus a diagnostic.
- Raw HTML is converted to supported native render nodes. Unsupported content is
  shown as source or a placeholder, never executed.

## 11. Performance controls

Only visible blocks and a small overscan region create QML delegates. Layout
height estimates maintain stable scrolling. Parsing is debounced but typing
updates the active block synchronously. Huge blocks fall back to source editing
when rich layout would exceed configured limits.

