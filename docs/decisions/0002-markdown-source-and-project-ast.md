# ADR-0002: Markdown source and project AST

Status: Proposed

## Context

Treating only source text as the editing model makes formatted block editing
difficult. Treating only a rich tree as authoritative can destroy intentional
Markdown formatting during serialization.

## Decision

The `.md` source is authoritative for persistence. An immutable project-owned AST
is authoritative for structural operations. AST nodes retain source spans and
original source slices. Unchanged nodes serialize from those slices; changed
nodes serialize through the project formatter and are reparsed.

cmark-gfm types remain private to the Markdown adapter.

## Consequences

- The model supports structural commands without normalizing the whole file.
- Source maps and stable-node reconciliation become critical infrastructure.
- Tests must cover both semantic equivalence and textual preservation.
- Some structural edits necessarily normalize their affected block and must be
  documented as such.

