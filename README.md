# Marknote

Marknote is a planned offline, cross-platform Markdown viewer and editor for
Windows, Linux, and macOS.

The project uses C++20, Qt 6.8+, QML, cmark-gfm, and a native document model. It
does not embed a browser engine for document editing or rendering.

## Project status

The current build contains the Qt/QML application shell, cmark-gfm rendering,
editable Markdown source, split/preview views, sidebar, save/Save As, HTML
and PDF export, formatting shortcuts, undo/redo, find/replace, heading outline,
dark appearance, image asset management, drag-and-drop insertion, and Qt Test
coverage for the controller/parser foundation.

A project-owned Document AST (built from cmark-gfm with source spans and stable
ID reconciliation) now drives outline navigation and block editing. Block mode
edits display text for paragraphs, headings, quotes, lists, tasks, code, tables,
thematic breaks, and YAML front matter, then serializes back to Markdown.
Workspace folder browsing, command palette (`Ctrl+Shift+P`), focus mode
(`F11`), list continuation in source mode, and task/strikethrough/link shortcuts
are available in the shell.

Richer table/media delegates, native reading renderer (non-HTML), math/diagram
adapters, and workspace content search remain future work. Table insertion,
crash-recovery snapshots, startup recovery, and external-file change detection
are enabled. Preview links resolve relative to the current Markdown file for
local resources and open through the platform desktop handler for external URLs.
Source mode uses a native Qt syntax highlighter for headings, emphasis, links,
inline code, and list/quote markers.

## Design documents

- [Product requirements](docs/01-product-requirements.md)
- [System architecture](docs/02-system-architecture.md)
- [Native block editor](docs/03-native-block-editor.md)
- [UI and interaction design](docs/04-ui-design.md)
- [Delivery and quality plan](docs/05-delivery-and-quality.md)
- [Architecture decisions](docs/decisions/README.md)
