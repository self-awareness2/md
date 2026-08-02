# Product requirements

## 1. Product definition

Marknote is an offline-first Markdown reader and editor. It targets users who
want Typora-like focus and presentation while retaining plain `.md` files and
native desktop behavior.

The application must work without an account, network connection, background
service, or embedded browser runtime.

## 2. Goals

1. Open, render, edit, and save Markdown reliably on Windows, Linux, and macOS.
2. Provide three document modes: Reading, Source, and Block Editing.
3. Preserve source Markdown wherever an edit does not require normalization.
4. Remain responsive for ordinary documents and usable for large documents.
5. Provide a restrained, accessible UI with light and dark themes.
6. Export documents to HTML and PDF in the first stable release.

## 3. Non-goals for version 1.0

- Cloud accounts, synchronization, collaboration, publishing, or telemetry.
- A general-purpose HTML editor or arbitrary JavaScript execution.
- Binary office-document fidelity comparable to Word or LibreOffice.
- A public native plugin ABI. Themes and declarative render extensions are in
  scope; binary plugins are postponed until the core ABI stabilizes.

## 4. Markdown compatibility profile

The normative base is CommonMark plus GitHub Flavored Markdown (GFM), parsed by
`cmark-gfm`. Marknote extensions are explicitly namespaced in settings and can
be disabled per workspace.

### Required syntax

- Paragraphs, ATX/setext headings, thematic breaks, block quotes.
- Ordered, unordered, nested, loose, and tight lists.
- Task lists, tables, strikethrough, autolinks, and tag filtering.
- Emphasis, strong emphasis, links, reference links, images, code spans.
- Indented and fenced code blocks with language metadata.
- Escapes, entities, hard/soft line breaks, raw inline and block HTML.
- YAML and TOML front matter.
- Footnotes, heading identifiers, table of contents directives.
- Inline and display mathematics.
- Mermaid and PlantUML fenced diagrams through optional local render adapters.

Raw HTML is displayed using a safe supported subset. Scripts, event handlers,
iframes, remote forms, and active embedded content are never executed.

## 5. Core workflows

### Open and read

- Open a file from the OS, recent list, drag and drop, or file dialog.
- Detect UTF-8, UTF-8 BOM, UTF-16, and selected legacy encodings.
- Show outline, backlinks within the current workspace, and document metadata.
- Navigate links with explicit confirmation for external URLs.

### Edit and save

- Edit in native source mode with syntax highlighting and structural commands.
- Edit in native block mode with formatting controls and Markdown shortcuts.
- Undo and redo across block operations without losing the selection.
- Autosave recovery data separately; never silently overwrite a newer file.
- Save atomically and preserve line-ending and encoding choices when possible.

### Manage resources

- Paste or drop images into a configurable assets directory.
- Keep relative paths relative to the document or workspace.
- Preview local images, SVG, audio, video, and linked PDF files safely.
- Warn about missing resources and optionally locate or relink them.

### Search and export

- Search and replace in the active document.
- Search filenames and contents in an opened workspace.
- Export self-contained or asset-linked HTML and paginated PDF.
- Print using the same paginated renderer as PDF export.

## 6. Quality attributes

| Attribute | Initial target |
| --- | --- |
| Startup | First window within 1.5 s on reference release hardware |
| Normal document | Smooth editing at 10,000 lines / 2 MB |
| Large document | Open and source-edit 20 MB without UI blocking |
| Input latency | P95 under 16 ms for ordinary paragraph editing |
| Data safety | No original-file truncation on failed or interrupted save |
| Recovery | At most 5 seconds of accepted edits lost after a crash |
| Accessibility | Full keyboard navigation and WCAG AA color contrast |
| Offline behavior | All core reading and editing features work offline |

Performance targets will be measured on documented reference devices; they are
not substitutes for correctness tests.

## 7. Acceptance definition for 1.0

Version 1.0 is complete only when the CommonMark and GFM conformance suites pass,
the three editing modes round-trip the supported syntax, crash-recovery and
external-change tests pass, and signed/installable packages are produced for all
three operating systems.

