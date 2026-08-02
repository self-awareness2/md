# ADR-0001: Native Qt architecture

Status: Proposed

## Context

The product must provide a polished cross-platform Markdown reader and a
Typora-like editing experience. A browser-backed editor would accelerate rich
text behavior but increases package size, attack surface, memory use, and the
distance between the editor model and Markdown source.

## Decision

Use C++20, Qt 6.8, QML, and native Qt text/layout APIs. Do not embed Qt WebEngine
or another browser engine in the core application. Parse CommonMark/GFM with
cmark-gfm and implement an AST-backed native block editor.

## Consequences

- Text input, accessibility, layout, selection, and block editing require more
  engineering than a DOM editor.
- The application has a smaller and more controllable runtime surface.
- Markdown structure and source-preservation rules remain explicit.
- Unsupported extension nodes must have a safe source-editing fallback.

