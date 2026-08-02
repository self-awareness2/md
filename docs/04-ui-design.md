# UI and interaction design

## 1. Visual direction

The interface is quiet and content-first. It avoids decorative cards, gradients,
oversized headings, and excessive rounded containers. Surfaces use neutral gray,
white/near-black, and a restrained accent color selected by the user or system.

Spacing uses a 4 px base grid. Standard corner radius is 4-6 px. Icons come from
one consistent symbol set and all unfamiliar icon-only controls have tooltips.

## 2. Main window

```text
+---------------------------------------------------------------+
| menu/title        document tabs                   window tools |
+--------------+--------------------------------+---------------+
| file/outline |                                | properties /  |
| sidebar      |       document canvas          | diagnostics   |
|              |                                | optional      |
|              |                                | inspector     |
+--------------+--------------------------------+---------------+
| status: mode | words | cursor | encoding | newline | zoom      |
+---------------------------------------------------------------+
```

Side panels are optional and resizable. Distraction-free mode hides everything
except the document and a transient top command affordance. The document column
has configurable width, line height, font, and page-like or continuous layout.

## 3. Modes

- Reading: no caret, optimized typography, link and outline navigation.
- Source: precise Markdown text editing with gutters and syntax highlighting.
- Block: formatted native structural editing with syntax revealed near the caret
  only where it aids comprehension.

Modes use a three-way segmented control and configurable shortcuts. Switching
modes preserves logical cursor position and viewport anchor.

## 4. Commands

Frequently used formatting commands appear in a compact contextual toolbar.
Application-wide actions are searchable in a command palette. Menus remain the
complete discoverable command surface and use native platform conventions.

Dialogs are reserved for consequential or multi-field actions. Save conflicts,
missing resources, and export failures provide a concrete resolution rather than
generic error text.

## 5. Responsive desktop layout

- Under 900 px: the right inspector becomes an overlay; only one sidebar opens.
- Under 680 px: tabs scroll, status items collapse by priority, document margins
  shrink, and toolbars expose overflow menus.
- No control may overlap document content at any supported window size.

The minimum supported window size is 640 x 480 logical pixels.

## 6. Themes and typography

Theme files define semantic tokens rather than widget-specific colors. Required
tokens include window, panel, document, text, muted text, border, selection,
accent, success, warning, error, link, code, and syntax categories.

Document themes are separate from application chrome themes. Default font stacks
are platform-aware, and CJK, emoji, mathematical, and monospace fallbacks are
explicit. Letter spacing remains zero unless a document theme explicitly needs
typographic adjustment.

## 7. Accessibility

- Every command is reachable by keyboard.
- Focus indicators remain visible in all themes.
- Icon-only buttons have accessible names and tooltips.
- Screen readers receive semantic block roles, heading levels, list positions,
  task state, link destination, table coordinates, and diagnostic descriptions.
- Reduced-motion and system high-contrast preferences are honored.
- Color is never the sole carrier of state.

## 8. Initial shortcut policy

Shortcuts follow each platform's conventions. User customization is stored as
command-to-key mappings. Conflicts are detected before applying changes. Vim or
other modal keymaps are extensions, not hard-coded into the editor core.

