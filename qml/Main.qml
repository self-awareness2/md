import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Marknote

ApplicationWindow {
    id: window
    width: 1280
    height: 820
    minimumWidth: 760
    minimumHeight: 520
    visible: true
    title: (appController.modified ? "* " : "")
           + (appController.hasDocument ? appController.currentFileName : qsTr("Untitled"))
           + qsTr(" - Marknote")

    Theme { id: theme; dark: window.darkMode }

    palette {
        window: theme.window
        windowText: theme.text
        base: theme.document
        text: theme.text
        button: theme.panel
        buttonText: theme.text
        highlight: theme.accent
        highlightedText: "#ffffff"
    }

    property int viewMode: 1 // 0: source, 1: preview, 2: split, 3: block
    property int sidebarTab: 0 // 0: workspace, 1: recent, 2: outline
    property bool sidebarVisible: true
    property bool darkMode: false
    property bool findVisible: false
    property bool focusMode: false
    property bool commandPaletteVisible: false
    property real fontScale: 1.0
    property string workspaceFilter: ""

    function zoomIn() { fontScale = Math.min(1.6, fontScale + 0.1) }
    function zoomOut() { fontScale = Math.max(0.7, fontScale - 0.1) }
    function resetZoom() { fontScale = 1.0 }
    function workspaceFileEntries() {
        const files = workspaceFilter.length === 0
                      ? appController.workspace.markdownFiles
                      : appController.workspace.searchFiles(workspaceFilter)
        const entries = []
        for (let i = 0; i < files.length; ++i) {
            const path = files[i]
            const relative = appController.workspace.relativePath(path)
            const slash = relative.lastIndexOf("/")
            entries.push({
                path: path,
                name: slash >= 0 ? relative.slice(slash + 1) : relative,
                dir: slash >= 0 ? relative.slice(0, slash) : ""
            })
        }
        entries.sort(function(a, b) {
            const dirCmp = a.dir.localeCompare(b.dir, undefined, { sensitivity: "base" })
            if (dirCmp !== 0)
                return dirCmp
            return a.name.localeCompare(b.name, undefined, { sensitivity: "base" })
        })
        return entries
    }
    function toggleFocusMode() {
        focusMode = !focusMode
        if (focusMode) {
            sidebarVisible = false
        }
    }
    function runCommand(id) {
        commandPaletteVisible = false
        switch (id) {
        case "new": appController.newDocument(); break
        case "open": appController.openFile(); break
        case "save": appController.save(); break
        case "workspace": appController.workspace.openFolder(); break
        case "prev-file": appController.openPreviousWorkspaceFile(); break
        case "next-file": appController.openNextWorkspaceFile(); break
        case "source": viewMode = 0; break
        case "split": viewMode = 2; break
        case "preview": viewMode = 1; break
        case "blocks": viewMode = 3; break
        case "focus": toggleFocusMode(); break
        case "dark": darkMode = !darkMode; break
        case "find": findVisible = true; break
        case "export-html": appController.exportHtmlAs(); break
        case "export-pdf": appController.exportPdfAs(); break
        case "image": appController.insertImage(); break
        case "heading": if (viewMode === 3) blockPane.convertActive("heading", 2); else editorPane.prefixLine("## "); break
        case "bullet": if (viewMode === 3) blockPane.convertActive("list"); else editorPane.prefixLine("- "); break
        case "task": if (viewMode === 3) blockPane.convertActive("task"); else editorPane.prefixLine("- [ ] "); break
        case "quote": if (viewMode === 3) blockPane.convertActive("quote"); else editorPane.prefixLine("> "); break
        case "code": if (viewMode === 3) blockPane.convertActive("code"); else editorPane.insertSnippet("```\n\n```"); break
        case "table": if (viewMode === 3) blockPane.insertTable(); else editorPane.insertTable(); break
        }
    }

    readonly property var commandItems: [
        { id: "new", title: qsTr("New Document"), shortcut: "Ctrl+N" },
        { id: "open", title: qsTr("Open File"), shortcut: "Ctrl+O" },
        { id: "save", title: qsTr("Save"), shortcut: "Ctrl+S" },
        { id: "workspace", title: qsTr("Open Workspace Folder"), shortcut: "Ctrl+Shift+O" },
        { id: "prev-file", title: qsTr("Previous Workspace File"), shortcut: "Ctrl+PageUp" },
        { id: "next-file", title: qsTr("Next Workspace File"), shortcut: "Ctrl+PageDown" },
        { id: "source", title: qsTr("Source Mode"), shortcut: "Ctrl+1" },
        { id: "split", title: qsTr("Split Mode"), shortcut: "Ctrl+2" },
        { id: "preview", title: qsTr("Preview Mode"), shortcut: "Ctrl+3" },
        { id: "blocks", title: qsTr("Block Editing Mode"), shortcut: "Ctrl+4" },
        { id: "focus", title: qsTr("Toggle Focus Mode"), shortcut: "F11" },
        { id: "dark", title: qsTr("Toggle Dark Appearance"), shortcut: "" },
        { id: "find", title: qsTr("Find in Document"), shortcut: "Ctrl+F" },
        { id: "heading", title: qsTr("Insert Heading"), shortcut: "" },
        { id: "bullet", title: qsTr("Insert Bullet List"), shortcut: "" },
        { id: "task", title: qsTr("Insert Task List"), shortcut: "" },
        { id: "quote", title: qsTr("Insert Quote"), shortcut: "" },
        { id: "code", title: qsTr("Insert Code Block"), shortcut: "" },
        { id: "table", title: qsTr("Insert Table"), shortcut: "" },
        { id: "image", title: qsTr("Insert Image"), shortcut: "" },
        { id: "export-html", title: qsTr("Export HTML"), shortcut: "" },
        { id: "export-pdf", title: qsTr("Export PDF"), shortcut: "" }
    ]

    Action { id: newAction; text: qsTr("New Document"); shortcut: StandardKey.New; onTriggered: appController.newDocument() }
    Action { id: openAction; text: qsTr("Open..."); shortcut: StandardKey.Open; onTriggered: appController.openFile() }
    Action { id: saveAction; text: qsTr("Save"); shortcut: StandardKey.Save; onTriggered: appController.save() }
    Action { id: saveAsAction; text: qsTr("Save As..."); shortcut: StandardKey.SaveAs; onTriggered: appController.saveAs() }
    Action { id: exportAction; text: qsTr("Export HTML..."); onTriggered: appController.exportHtmlAs() }
    Action { id: exportPdfAction; text: qsTr("Export PDF..."); onTriggered: appController.exportPdfAs() }
    Action { id: undoAction; text: qsTr("Undo"); shortcut: StandardKey.Undo; onTriggered: viewMode === 3 ? blockPane.undo() : editorPane.undo() }
    Action { id: redoAction; text: qsTr("Redo"); shortcut: StandardKey.Redo; onTriggered: viewMode === 3 ? blockPane.redo() : editorPane.redo() }
    Action { id: findAction; text: qsTr("Find"); shortcut: StandardKey.Find; onTriggered: findVisible = true }
    Action { id: commandPaletteAction; text: qsTr("Command Palette"); shortcut: "Ctrl+Shift+P"; onTriggered: commandPaletteVisible = true }
    Action { id: focusModeAction; text: qsTr("Focus Mode"); shortcut: "F11"; onTriggered: toggleFocusMode() }
    Action { id: openWorkspaceAction; text: qsTr("Open Workspace..."); shortcut: "Ctrl+Shift+O"; onTriggered: appController.workspace.openFolder() }
    Action {
        id: previousWorkspaceFileAction
        text: qsTr("Previous File")
        shortcut: "Ctrl+PageUp"
        enabled: appController.canOpenPreviousWorkspaceFile
        onTriggered: appController.openPreviousWorkspaceFile()
    }
    Action {
        id: nextWorkspaceFileAction
        text: qsTr("Next File")
        shortcut: "Ctrl+PageDown"
        enabled: appController.canOpenNextWorkspaceFile
        onTriggered: appController.openNextWorkspaceFile()
    }
    Action { id: sourceModeAction; text: qsTr("Source"); shortcut: "Ctrl+1"; onTriggered: viewMode = 0 }
    Action { id: splitModeAction; text: qsTr("Split"); shortcut: "Ctrl+2"; onTriggered: viewMode = 2 }
    Action { id: previewModeAction; text: qsTr("Preview"); shortcut: "Ctrl+3"; onTriggered: viewMode = 1 }
    Action { id: blockModeAction; text: qsTr("Blocks"); shortcut: "Ctrl+4"; onTriggered: viewMode = 3 }
    Action { id: zoomInAction; text: qsTr("Zoom In"); shortcut: "Ctrl+="; onTriggered: zoomIn() }
    Action { id: zoomOutAction; text: qsTr("Zoom Out"); shortcut: "Ctrl+-"; onTriggered: zoomOut() }
    Action { id: resetZoomAction; text: qsTr("Reset Zoom"); shortcut: "Ctrl+0"; onTriggered: resetZoom() }

    menuBar: MenuBar {
        visible: !focusMode
        Menu {
            title: qsTr("File")
            Action { text: newAction.text; shortcut: newAction.shortcut; onTriggered: newAction.trigger() }
            Action { text: openAction.text; shortcut: openAction.shortcut; onTriggered: openAction.trigger() }
            Action { text: openWorkspaceAction.text; shortcut: openWorkspaceAction.shortcut; onTriggered: openWorkspaceAction.trigger() }
            Action {
                text: previousWorkspaceFileAction.text
                shortcut: previousWorkspaceFileAction.shortcut
                enabled: previousWorkspaceFileAction.enabled
                onTriggered: previousWorkspaceFileAction.trigger()
            }
            Action {
                text: nextWorkspaceFileAction.text
                shortcut: nextWorkspaceFileAction.shortcut
                enabled: nextWorkspaceFileAction.enabled
                onTriggered: nextWorkspaceFileAction.trigger()
            }
            MenuSeparator {}
            Action { text: saveAction.text; shortcut: saveAction.shortcut; onTriggered: saveAction.trigger() }
            Action { text: saveAsAction.text; shortcut: saveAsAction.shortcut; onTriggered: saveAsAction.trigger() }
            Action { text: exportAction.text; onTriggered: exportAction.trigger() }
            Action { text: exportPdfAction.text; onTriggered: exportPdfAction.trigger() }
            MenuSeparator {}
            Action { text: qsTr("Close Document"); onTriggered: appController.closeDocument() }
        }
        Menu {
            title: qsTr("Edit")
            Action { text: undoAction.text; shortcut: undoAction.shortcut; onTriggered: undoAction.trigger() }
            Action { text: redoAction.text; shortcut: redoAction.shortcut; onTriggered: redoAction.trigger() }
            MenuSeparator {}
            Action { text: findAction.text; shortcut: findAction.shortcut; onTriggered: findAction.trigger() }
            Action { text: commandPaletteAction.text; shortcut: commandPaletteAction.shortcut; onTriggered: commandPaletteAction.trigger() }
            MenuSeparator {}
            Action { text: qsTr("Bold"); shortcut: "Ctrl+B"; onTriggered: editorPane.wrapSelection("**", "**") }
            Action { text: qsTr("Italic"); shortcut: "Ctrl+I"; onTriggered: editorPane.wrapSelection("*", "*") }
            Action { text: qsTr("Strikethrough"); shortcut: "Ctrl+Shift+X"; onTriggered: editorPane.wrapSelection("~~", "~~") }
            Action { text: qsTr("Inline Code"); shortcut: "Ctrl+`"; onTriggered: editorPane.wrapSelection("`", "`") }
            Action { text: qsTr("Link"); shortcut: "Ctrl+K"; onTriggered: editorPane.wrapSelection("[", "](url)") }
        }
        Menu {
            title: qsTr("View")
            Action { text: qsTr("Source"); checkable: true; checked: viewMode === 0; onTriggered: viewMode = 0 }
            Action { text: qsTr("Split"); checkable: true; checked: viewMode === 2; onTriggered: viewMode = 2 }
            Action { text: qsTr("Preview"); checkable: true; checked: viewMode === 1; onTriggered: viewMode = 1 }
            Action { text: qsTr("Block Editing"); checkable: true; checked: viewMode === 3; onTriggered: viewMode = 3 }
            MenuSeparator {}
            Action { text: qsTr("Toggle Sidebar"); shortcut: "Ctrl+Shift+B"; onTriggered: sidebarVisible = !sidebarVisible }
            Action { text: focusModeAction.text; shortcut: focusModeAction.shortcut; checkable: true; checked: focusMode; onTriggered: toggleFocusMode() }
            Action { text: qsTr("Toggle Dark Appearance"); checkable: true; checked: darkMode; onTriggered: darkMode = !darkMode }
            MenuSeparator {}
            Action { text: zoomInAction.text; shortcut: zoomInAction.shortcut; onTriggered: zoomInAction.trigger() }
            Action { text: zoomOutAction.text; shortcut: zoomOutAction.shortcut; onTriggered: zoomOutAction.trigger() }
            Action { text: resetZoomAction.text; shortcut: resetZoomAction.shortcut; onTriggered: resetZoomAction.trigger() }
        }
    }

    header: ToolBar {
        id: toolbar
        visible: !focusMode
        height: focusMode ? 0 : 54
        background: Rectangle { color: theme.panel; border.color: theme.divider; border.width: 1 }
        contentItem: RowLayout {
            spacing: 6
            Image {
                source: "qrc:/marknote/resources/icons/marknote.png"
                sourceSize.width: 24
                sourceSize.height: 24
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                Layout.leftMargin: 8
                Layout.rightMargin: 2
                smooth: true
                mipmap: true
            }
            ToolButton {
                text: sidebarVisible ? "◂" : "▸"
                display: AbstractButton.TextOnly
                onClicked: sidebarVisible = !sidebarVisible
                ToolTip.visible: hovered
                ToolTip.text: sidebarVisible ? qsTr("Hide sidebar (Ctrl+Shift+B)") : qsTr("Show sidebar (Ctrl+Shift+B)")
            }
            ToolButton {
                icon.name: "document-new"
                display: AbstractButton.IconOnly
                action: newAction
                ToolTip.visible: hovered
                ToolTip.text: qsTr("New document")
            }
            ToolButton {
                icon.name: "document-open"
                display: AbstractButton.IconOnly
                action: openAction
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Open document")
            }
            ToolButton {
                icon.name: "document-save"
                display: AbstractButton.IconOnly
                action: saveAction
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Save document")
            }
            ToolButton { icon.name: "edit-undo"; display: AbstractButton.IconOnly; action: undoAction; ToolTip.visible: hovered; ToolTip.text: qsTr("Undo") }
            ToolButton { icon.name: "edit-redo"; display: AbstractButton.IconOnly; action: redoAction; ToolTip.visible: hovered; ToolTip.text: qsTr("Redo") }
            ToolButton { text: "-"; display: AbstractButton.TextOnly; action: zoomOutAction; ToolTip.visible: hovered; ToolTip.text: qsTr("Zoom out") }
            Label { text: qsTr("%1%").arg(Math.round(window.fontScale * 100)); color: theme.mutedText; font.pixelSize: 11 }
            ToolButton { text: "+"; display: AbstractButton.TextOnly; action: zoomInAction; ToolTip.visible: hovered; ToolTip.text: qsTr("Zoom in") }
            Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 22; color: theme.divider; Layout.leftMargin: 4; Layout.rightMargin: 4 }
            Label {
                text: appController.hasDocument ? appController.currentFileName : qsTr("Untitled")
                color: theme.text
                font.pixelSize: 14
                font.weight: Font.DemiBold
                elide: Text.ElideMiddle
                Layout.maximumWidth: 360
            }
            RowLayout {
                visible: appController.workspace.hasWorkspace && appController.workspaceFileIndex > 0
                spacing: 2
                ToolButton {
                    text: "‹"
                    display: AbstractButton.TextOnly
                    enabled: previousWorkspaceFileAction.enabled
                    onClicked: previousWorkspaceFileAction.trigger()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Previous Markdown file (Ctrl+PageUp)")
                }
                Label {
                    text: qsTr("%1 / %2").arg(appController.workspaceFileIndex).arg(appController.workspaceFileCount)
                    color: theme.mutedText
                    font.pixelSize: 11
                }
                ToolButton {
                    text: "›"
                    display: AbstractButton.TextOnly
                    enabled: nextWorkspaceFileAction.enabled
                    onClicked: nextWorkspaceFileAction.trigger()
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Next Markdown file (Ctrl+PageDown)")
                }
            }
            Label { text: appController.modified ? qsTr("Unsaved") : ""; color: theme.accent; font.pixelSize: 12 }
            Label { text: appController.externalChangeDetected ? qsTr("Changed on disk") : ""; color: "#ff9f0a"; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            RowLayout {
                spacing: 2
                ToolButton { text: qsTr("Edit"); checkable: true; checked: viewMode === 0; onClicked: viewMode = 0 }
                ToolButton { text: qsTr("Split"); checkable: true; checked: viewMode === 2; onClicked: viewMode = 2 }
                ToolButton { text: qsTr("Preview"); checkable: true; checked: viewMode === 1; onClicked: viewMode = 1 }
                ToolButton { text: qsTr("Blocks"); checkable: true; checked: viewMode === 3; onClicked: viewMode = 3 }
            }
            RowLayout {
                visible: viewMode === 0 || viewMode === 2
                spacing: 0
                ToolButton { text: "B"; font.bold: true; onClicked: editorPane.wrapSelection("**", "**"); ToolTip.visible: hovered; ToolTip.text: qsTr("Bold") }
                ToolButton { text: "I"; font.italic: true; onClicked: editorPane.wrapSelection("*", "*"); ToolTip.visible: hovered; ToolTip.text: qsTr("Italic") }
                ToolButton { text: "S"; onClicked: editorPane.wrapSelection("~~", "~~"); ToolTip.visible: hovered; ToolTip.text: qsTr("Strikethrough") }
                ToolButton { text: "`"; onClicked: editorPane.wrapSelection("`", "`"); ToolTip.visible: hovered; ToolTip.text: qsTr("Inline code") }
                ToolButton { text: "H"; onClicked: editorPane.prefixLine("## "); ToolTip.visible: hovered; ToolTip.text: qsTr("Heading") }
                ToolButton { text: "-"; onClicked: editorPane.prefixLine("- "); ToolTip.visible: hovered; ToolTip.text: qsTr("Bullet list") }
                ToolButton { text: "☑"; onClicked: editorPane.prefixLine("- [ ] "); ToolTip.visible: hovered; ToolTip.text: qsTr("Task list") }
                ToolButton { text: "T"; onClicked: editorPane.insertTable(); ToolTip.visible: hovered; ToolTip.text: qsTr("Insert table") }
            }
            RowLayout {
                visible: viewMode === 3
                spacing: 0
                ToolButton { text: "H"; onClicked: blockPane.convertActive("heading", 2); ToolTip.visible: hovered; ToolTip.text: qsTr("Heading block") }
                ToolButton { text: "-"; onClicked: blockPane.convertActive("list"); ToolTip.visible: hovered; ToolTip.text: qsTr("List block") }
                ToolButton { text: "☑"; onClicked: blockPane.convertActive("task"); ToolTip.visible: hovered; ToolTip.text: qsTr("Task block") }
                ToolButton { text: ">"; onClicked: blockPane.convertActive("quote"); ToolTip.visible: hovered; ToolTip.text: qsTr("Quote block") }
                ToolButton { text: "</>"; onClicked: blockPane.convertActive("code"); ToolTip.visible: hovered; ToolTip.text: qsTr("Code block") }
                ToolButton { text: "T"; onClicked: blockPane.insertTable(); ToolTip.visible: hovered; ToolTip.text: qsTr("Insert table block") }
            }
            ToolButton { text: qsTr("Focus"); checkable: true; checked: focusMode; onClicked: toggleFocusMode(); ToolTip.visible: hovered; ToolTip.text: qsTr("Focus mode") }
            ToolButton { text: "⌘"; onClicked: commandPaletteVisible = true; ToolTip.visible: hovered; ToolTip.text: qsTr("Command palette") }
            ToolButton {
                icon.name: "document-export"
                display: AbstractButton.IconOnly
                action: exportAction
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Export HTML")
            }
            ToolButton { icon.name: "edit-find"; display: AbstractButton.IconOnly; action: findAction; ToolTip.visible: hovered; ToolTip.text: qsTr("Find") }
            ToolButton { visible: appController.externalChangeDetected; text: qsTr("Reload"); onClicked: appController.reloadFromDisk(); ToolTip.visible: hovered; ToolTip.text: qsTr("Reload file from disk") }
            ToolButton { icon.name: "insert-image"; display: AbstractButton.IconOnly; onClicked: appController.insertImage(); ToolTip.visible: hovered; ToolTip.text: qsTr("Insert image") }
            ToolButton { icon.name: "document-print"; display: AbstractButton.IconOnly; action: exportPdfAction; ToolTip.visible: hovered; ToolTip.text: qsTr("Export PDF") }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            id: sidebarHost
            visible: !focusMode
            Layout.preferredWidth: sidebarVisible ? 268 : 32
            Layout.minimumWidth: sidebarVisible ? 220 : 32
            Layout.maximumWidth: sidebarVisible ? 420 : 32
            Layout.fillHeight: true
            clip: true

            Behavior on Layout.preferredWidth {
                NumberAnimation { duration: 160; easing.type: Easing.OutCubic }
            }

            // Collapsed rail — click to restore sidebar
            Rectangle {
                anchors.fill: parent
                visible: !sidebarVisible
                color: theme.panel
                border.color: theme.divider
                border.width: 1

                ToolButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    width: 28
                    height: 28
                    text: "›"
                    display: AbstractButton.TextOnly
                    onClicked: sidebarVisible = true
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Show sidebar (Ctrl+Shift+B)")
                }
            }

            Pane {
                anchors.fill: parent
                visible: sidebarVisible
                padding: 16
                background: Rectangle { color: theme.panel; border.color: theme.divider; border.width: 1 }
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        ToolButton {
                            text: qsTr("Files")
                            checkable: true
                            checked: sidebarTab === 0
                            Layout.fillWidth: true
                            onClicked: sidebarTab = 0
                        }
                        ToolButton {
                            text: qsTr("Recent")
                            checkable: true
                            checked: sidebarTab === 1
                            Layout.fillWidth: true
                            onClicked: sidebarTab = 1
                        }
                        ToolButton {
                            text: qsTr("Outline")
                            checkable: true
                            checked: sidebarTab === 2
                            Layout.fillWidth: true
                            onClicked: sidebarTab = 2
                        }
                        ToolButton {
                            text: "«"
                            display: AbstractButton.TextOnly
                            onClicked: sidebarVisible = false
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Hide sidebar (Ctrl+Shift+B)")
                        }
                    }

                    ColumnLayout {
                        visible: sidebarTab === 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 10
                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: appController.workspace.hasWorkspace ? appController.workspace.rootName : qsTr("No folder open")
                                color: theme.text
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                            ToolButton {
                                text: appController.workspace.hasWorkspace ? qsTr("↻") : qsTr("+")
                                onClicked: appController.workspace.hasWorkspace ? appController.workspace.refresh() : appController.workspace.openFolder()
                                ToolTip.visible: hovered
                                ToolTip.text: appController.workspace.hasWorkspace ? qsTr("Refresh workspace") : qsTr("Open workspace folder")
                            }
                        }
                        TextField {
                            visible: appController.workspace.hasWorkspace
                            Layout.fillWidth: true
                            placeholderText: qsTr("Filter files")
                            text: workspaceFilter
                            onTextChanged: workspaceFilter = text
                        }
                        ListView {
                            id: workspaceList
                            visible: appController.workspace.hasWorkspace
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            // Depend on markdownFiles/filter so the grouped list refreshes.
                            model: {
                                const _files = appController.workspace.markdownFiles
                                const _filter = workspaceFilter
                                return window.workspaceFileEntries()
                            }
                            section.property: "dir"
                            section.criteria: ViewSection.FullString
                            section.delegate: Item {
                                required property string section
                                width: ListView.view ? ListView.view.width : 0
                                height: 22
                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    leftPadding: 2
                                    text: parent.section.length === 0 ? qsTr("Root") : parent.section
                                    color: theme.faintText
                                    font.pixelSize: 10
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideMiddle
                                    width: parent.width - 4
                                }
                            }
                            delegate: ItemDelegate {
                                width: ListView.view.width
                                height: 28
                                text: modelData.name
                                font.pixelSize: 12
                                highlighted: appController.currentFile === modelData.path
                                ToolTip.visible: hovered
                                ToolTip.text: modelData.dir.length === 0 ? modelData.name : (modelData.dir + "/" + modelData.name)
                                ToolTip.delay: 400
                                onClicked: appController.openPath(modelData.path)
                            }
                        }
                        Label {
                            visible: !appController.workspace.hasWorkspace
                            text: qsTr("Open a folder to browse Markdown files.")
                            color: theme.mutedText
                            font.pixelSize: 11
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    ColumnLayout {
                        visible: sidebarTab === 1
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 10
                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: qsTr("Recent files")
                                color: theme.mutedText
                                font.pixelSize: 12
                                Layout.fillWidth: true
                            }
                            ToolButton {
                                text: qsTr("Clear")
                                visible: appController.recentFiles.length > 0
                                onClicked: appController.clearRecentFiles()
                            }
                        }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: appController.recentFiles
                            clip: true
                            delegate: ItemDelegate {
                                width: ListView.view.width
                                height: 28
                                text: modelData.split(/[\\/]/).pop()
                                font.pixelSize: 11
                                onClicked: appController.openPath(modelData)
                            }
                            Label {
                                anchors.centerIn: parent
                                visible: parent.count === 0
                                text: qsTr("No recent files")
                                color: theme.mutedText
                                font.pixelSize: 11
                            }
                        }
                    }

                    ColumnLayout {
                        visible: sidebarTab === 2
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 10
                        Label {
                            text: qsTr("Document outline")
                            color: theme.mutedText
                            font.pixelSize: 12
                            Layout.fillWidth: true
                        }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: appController.documentOutline
                            clip: true
                            delegate: ItemDelegate {
                                width: ListView.view.width
                                height: 30
                                leftPadding: 8 + ((modelData.level || 1) - 1) * 12
                                text: modelData.title
                                font.pixelSize: 12
                                onClicked: {
                                    if (viewMode === 3) {
                                        blockPane.focusNear(modelData.position)
                                    } else {
                                        editorPane.revealPosition(modelData.position)
                                        if (viewMode === 1) viewMode = 2
                                    }
                                }
                            }
                            Label {
                                anchors.centerIn: parent
                                visible: parent.count === 0
                                text: qsTr("No headings in this document")
                                color: theme.mutedText
                                font.pixelSize: 11
                            }
                        }
                    }

                    Label {
                        text: qsTr("LOCAL DOCUMENT")
                        color: theme.faintText
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                    }
                    Label {
                        text: appController.hasDocument ? appController.currentFile : qsTr("Open a Markdown file to get started")
                        color: theme.mutedText
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }
            }
        }

        Pane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0
            background: Rectangle { color: theme.window }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                visible: appController.hasDocument

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    Layout.leftMargin: 28; Layout.rightMargin: 28
                    Label { text: viewMode === 0 ? qsTr("Markdown") : viewMode === 1 ? qsTr("Preview") : viewMode === 3 ? qsTr("Block Editing") : qsTr("Markdown / Preview"); color: theme.mutedText; font.pixelSize: 12; Layout.fillWidth: true }
                    Label { text: qsTr("UTF-8"); color: theme.faintText; font.pixelSize: 11 }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0
                    EditorPane {
                        id: editorPane
                        visible: viewMode === 0 || viewMode === 2
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: viewMode === 2 ? 2 : 1
                        Layout.minimumWidth: viewMode === 2 ? 220 : 0
                    }
                    Rectangle {
                        visible: viewMode === 2
                        Layout.preferredWidth: 1
                        Layout.fillHeight: true
                        color: theme.divider
                    }
                    PreviewPane {
                        visible: viewMode === 1 || viewMode === 2
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: viewMode === 2 ? 3 : 1
                        Layout.minimumWidth: viewMode === 2 ? 280 : 0
                    }
                    BlockPane {
                        id: blockPane
                        visible: viewMode === 3
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 1
                    }
                }
                Label { text: qsTr("%1 words  /  %2 characters").arg(editorPane.wordCount).arg(appController.documentText.length); color: theme.faintText; font.pixelSize: 11; Layout.alignment: Qt.AlignRight; Layout.rightMargin: 28; Layout.bottomMargin: 10 }
            }

            ColumnLayout {
                anchors.centerIn: parent; width: Math.min(parent.width - 64, 520); spacing: 14; visible: !appController.hasDocument
                Image {
                    source: "qrc:/marknote/resources/icons/marknote.png"
                    sourceSize.width: 72
                    sourceSize.height: 72
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    Layout.alignment: Qt.AlignHCenter
                    smooth: true
                    mipmap: true
                }
                Label {
                    text: qsTr("Marknote")
                    font.pixelSize: 28
                    font.weight: Font.DemiBold
                    color: theme.text
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: qsTr("A quiet place for your words.")
                    font.pixelSize: 15
                    color: theme.mutedText
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: qsTr("Create a document or open an existing Markdown file to begin.")
                    color: theme.mutedText
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                RowLayout { Layout.alignment: Qt.AlignHCenter; spacing: 8
                    Button { text: qsTr("New document"); icon.name: "document-new"; onClicked: appController.newDocument() }
                    Button { text: qsTr("Open file"); icon.name: "document-open"; highlighted: true; onClicked: appController.openFile() }
                }
            }
        }
    }

    DropArea {
        id: documentDropArea
        anchors.fill: parent
        visible: !appController.hasDocument
        keys: ["text/uri-list"]
        onDropped: function(drop) {
            if (!drop.urls || drop.urls.length === 0) {
                return
            }
            for (const url of drop.urls) {
                const localPath = url.toLocalFile()
                if (/\.(?:md|markdown)$/i.test(localPath)) {
                    appController.openPath(localPath)
                    drop.acceptProposedAction()
                    return
                }
            }
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 24
            radius: theme.radius
            color: theme.accentSoft
            border.color: theme.accent
            border.width: 2
            visible: documentDropArea.containsDrag
            opacity: 0.9
            Label {
                anchors.centerIn: parent
                text: qsTr("Drop a Markdown file to open")
                color: theme.accent
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }
        }
    }

    component EditorPane: ScrollView {
        property int wordCount: editor.wordCount
        function undo() { editor.undo() }
        function redo() { editor.redo() }
        function insertSnippet(snippet) {
            const start = editor.selectionStart
            const end = editor.selectionEnd
            editor.remove(start, end)
            editor.insert(start, snippet)
            editor.cursorPosition = start + snippet.length
            editor.forceActiveFocus()
        }
        function insertTable() {
            insertSnippet("| Column 1 | Column 2 |\n| --- | --- |\n|  |  |")
        }
        function revealPosition(position) {
            editor.cursorPosition = position
            editor.forceActiveFocus()
        }
        function findNext(query) {
            if (query.length === 0) {
                return
            }
            let index = editor.text.indexOf(query, Math.max(editor.cursorPosition, 0))
            if (index < 0) {
                index = editor.text.indexOf(query)
            }
            if (index >= 0) {
                editor.select(index, index + query.length)
                editor.forceActiveFocus()
            }
        }
        function replaceCurrent(query, replacement) {
            if (query.length === 0 || editor.selectedText !== query) {
                findNext(query)
                return
            }
            const index = editor.selectionStart
            editor.remove(index, editor.selectionEnd)
            editor.insert(index, replacement)
            editor.select(index, index + replacement.length)
        }
        function replaceAll(query, replacement) {
            if (query.length === 0) {
                return
            }
            let value = editor.text
            value = value.split(query).join(replacement)
            editor.remove(0, editor.length)
            editor.insert(0, value)
            editor.forceActiveFocus()
        }
        function wrapSelection(before, after) {
            const start = editor.selectionStart
            const end = editor.selectionEnd
            const selected = editor.selectedText
            editor.remove(start, end)
            editor.insert(start, before + selected + after)
            editor.select(start + before.length, start + before.length + selected.length)
            editor.forceActiveFocus()
        }
        function prefixLine(prefix) {
            const start = editor.selectionStart
            const lineStart = editor.text.lastIndexOf("\n", Math.max(0, start - 1)) + 1
            editor.insert(lineStart, prefix)
            editor.cursorPosition = start + prefix.length
            editor.forceActiveFocus()
        }
        function continueListOrExit(event) {
            const pos = editor.cursorPosition
            const lineStart = editor.text.lastIndexOf("\n", Math.max(0, pos - 1)) + 1
            const lineEnd = editor.text.indexOf("\n", pos)
            const line = editor.text.substring(lineStart, lineEnd < 0 ? editor.text.length : lineEnd)
            const match = /^(\s*)([-+*]|\d+[.)])(\s+(?:\[[ xX]\]\s+)?)(.*)$/.exec(line)
            if (!match) {
                return false
            }
            const indent = match[1]
            const marker = match[2]
            const taskOrSpace = match[3]
            const content = match[4]
            if (content.length === 0) {
                editor.remove(lineStart, pos)
                event.accepted = true
                return true
            }
            let nextMarker = marker
            const ordered = /^(\d+)([.)])$/.exec(marker)
            if (ordered) {
                nextMarker = String(Number(ordered[1]) + 1) + ordered[2]
            }
            let nextTask = taskOrSpace
            if (/\[[ xX]\]/.test(taskOrSpace)) {
                nextTask = " [ ] "
            }
            const insertion = "\n" + indent + nextMarker + nextTask
            editor.insert(pos, insertion)
            editor.cursorPosition = pos + insertion.length
            event.accepted = true
            return true
        }
        id: editorScroll
        clip: true
        TextArea {
            id: editor
            property int wordCount: text.trim().length === 0 ? 0 : text.trim().split(/\s+/).length
            text: appController.documentText
            selectByMouse: true
            wrapMode: TextEdit.Wrap
            color: theme.text
            selectionColor: theme.selection
            selectedTextColor: theme.text
            font.family: "Segoe UI"
            font.pixelSize: 16 * window.fontScale
            leftPadding: focusMode ? Math.max(48, (window.width - 760) / 2) : 34
            rightPadding: focusMode ? Math.max(48, (window.width - 760) / 2) : 34
            topPadding: focusMode ? 48 : 24
            bottomPadding: 40
            background: Rectangle { color: theme.document }
            onTextChanged: if (activeFocus) appController.setDocumentText(text)
            Keys.onPressed: function(event) {
                if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_S) { appController.save(); event.accepted = true }
                else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_B) { editorPane.wrapSelection("**", "**"); event.accepted = true }
                else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_I) { editorPane.wrapSelection("*", "*"); event.accepted = true }
                else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (!(event.modifiers & Qt.ControlModifier) && editorPane.continueListOrExit(event)) {
                        return
                    }
                }
            }
        }
        MarkdownSyntaxHighlighter {
            document: editor.textDocument
            dark: window.darkMode
        }
        DropArea {
            anchors.fill: parent
            keys: ["text/uri-list"]
            onDropped: function(drop) {
                if (!drop.urls || drop.urls.length === 0) {
                    return
                }
                const localPath = drop.urls[0].toLocalFile()
                const relativePath = appController.prepareImage(localPath)
                if (relativePath.length > 0) {
                    editorPane.insertSnippet("![image](" + relativePath + ")")
                }
                drop.acceptProposedAction()
            }
        }
    }

    component BlockPane: ScrollView {
        property int activeIndex: -1
        property var activeBlock: activeIndex >= 0 && activeIndex < appController.documentBlocks.length
                                  ? appController.documentBlocks[activeIndex] : null
        function undo() {
            const item = activeIndex >= 0 ? listView.itemAtIndex(activeIndex) : null
            if (item && item.editorObject) item.editorObject.undo()
        }
        function redo() {
            const item = activeIndex >= 0 ? listView.itemAtIndex(activeIndex) : null
            if (item && item.editorObject) item.editorObject.redo()
        }
        function commitBlock(blockId, value) {
            appController.updateBlockDisplay(blockId, value)
        }
        function convertActive(kind, level) {
            if (!activeBlock) {
                return
            }
            appController.setBlockKind(activeBlock.id, kind, level || 0)
        }
        function addBlock() {
            const afterId = activeBlock ? activeBlock.id : 0
            appController.insertBlockAfter(afterId, "paragraph")
            listView.currentIndex = Math.min(activeIndex + 1, appController.documentBlocks.length - 1)
        }
        function insertTable() {
            const afterId = activeBlock ? activeBlock.id : 0
            appController.insertBlockAfter(afterId, "table")
        }
        function focusNear(position) {
            const blocks = appController.documentBlocks
            let best = 0
            for (let i = 0; i < blocks.length; ++i) {
                if (blocks[i].begin <= position) {
                    best = i
                }
            }
            listView.currentIndex = best
            listView.positionViewAtIndex(best, ListView.Center)
        }
        clip: true
        ListView {
            id: listView
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: addButton.top
            anchors.topMargin: 18
            anchors.bottomMargin: 12
            model: appController.documentBlocks
            spacing: 12
            delegate: Item {
                id: blockDelegate
                property var editorObject: null
                property var block: modelData
                width: ListView.view.width
                height: kind === "rule" ? 36 : Math.max(58, blockRow.implicitHeight + 8)
                readonly property string kind: block.kind
                readonly property int level: block.level || 0
                RowLayout {
                    id: blockRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: focusMode ? Math.max(40, (window.width - 780) / 2) : 40
                    anchors.rightMargin: focusMode ? Math.max(40, (window.width - 780) / 2) : 40
                    anchors.top: parent.top
                    spacing: 8
                    CheckBox {
                        visible: kind === "list" && block.task
                        checked: !!block.taskChecked
                        onToggled: appController.toggleTaskChecked(block.id)
                    }
                    Label {
                        visible: kind === "rule"
                        text: "—"
                        color: theme.faintText
                        font.pixelSize: 18
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }
                    TextArea {
                        id: blockEditor
                        visible: kind !== "rule"
                        Layout.fillWidth: true
                        text: block.displayText // initial; Binding keeps it fresh while unfocused
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        readOnly: kind === "frontmatter"
                        color: theme.text
                        selectionColor: theme.selection
                        selectedTextColor: theme.text
                        font.family: kind === "code" || kind === "frontmatter" ? "Consolas" : "Segoe UI"
                        font.pixelSize: (kind === "heading" ? Math.max(18, 32 - level * 2) : 16) * window.fontScale
                        font.bold: kind === "heading"
                        font.italic: kind === "quote"
                        leftPadding: kind === "quote" ? 18 : 12
                        rightPadding: 12
                        topPadding: 10
                        bottomPadding: 10
                        background: Rectangle {
                            color: kind === "code" || kind === "frontmatter" ? theme.codeBackground
                                   : kind === "table" ? theme.accentSoft
                                   : kind === "quote" ? theme.panel
                                   : theme.document
                            radius: theme.smallRadius
                            border.color: blockEditor.activeFocus ? theme.accent : (kind === "quote" ? theme.accent : theme.divider)
                            border.width: blockEditor.activeFocus ? 2 : (kind === "quote" ? 0 : 1)
                            Rectangle {
                                visible: kind === "quote"
                                width: 3
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                color: theme.accent
                            }
                        }
                        Binding {
                            target: blockEditor
                            property: "text"
                            value: block.displayText
                            when: !blockEditor.activeFocus
                            restoreMode: Binding.RestoreNone
                        }
                        onActiveFocusChanged: {
                            if (activeFocus) {
                                blockPane.activeIndex = index
                            } else if (text !== block.displayText) {
                                blockPane.commitBlock(block.id, text)
                            }
                        }
                        Keys.onPressed: function(event) {
                            if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)) {
                                if (text !== block.displayText) {
                                    blockPane.commitBlock(block.id, text)
                                }
                                blockPane.addBlock()
                                event.accepted = true
                            } else if (event.key === Qt.Key_Backspace && blockEditor.text.length === 0 && appController.documentBlocks.length > 1) {
                                appController.deleteBlock(block.id)
                                event.accepted = true
                            }
                        }
                        onEditingFinished: {
                            const value = text
                            if (value !== block.displayText) {
                                blockPane.commitBlock(block.id, value)
                            }
                            if (kind === "paragraph") {
                                if (/^#{1,6}\s/.test(value)) {
                                    const level = /^(#+)/.exec(value)[1].length
                                    const body = value.replace(/^#{1,6}\s+/, "")
                                    appController.updateBlockDisplay(block.id, body)
                                    appController.setBlockKind(block.id, "heading", level)
                                } else if (/^[-*+]\s+\[[ xX]\]\s/.test(value)) {
                                    appController.updateBlockDisplay(block.id, value.replace(/^[-*+]\s+\[[ xX]\]\s+/, ""))
                                    appController.setBlockKind(block.id, "task")
                                } else if (/^[-*+]\s+/.test(value)) {
                                    appController.updateBlockDisplay(block.id, value.replace(/^[-*+]\s+/, ""))
                                    appController.setBlockKind(block.id, "list")
                                } else if (/^>\s?/.test(value)) {
                                    appController.updateBlockDisplay(block.id, value.replace(/^>\s?/, ""))
                                    appController.setBlockKind(block.id, "quote")
                                } else if (/^```/.test(value)) {
                                    appController.updateBlockDisplay(block.id, value.replace(/^```[^\n]*\n?/, "").replace(/\n?```$/, ""))
                                    appController.setBlockKind(block.id, "code")
                                }
                            }
                        }
                    }
                }
                Component.onCompleted: editorObject = blockEditor
                Component.onDestruction: editorObject = null
            }
        }
        Button {
            id: addButton
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            text: qsTr("Add block")
            icon.name: "list-add"
            onClicked: blockPane.addBlock()
        }
    }

    component PreviewPane: Item {
        Flickable {
            id: previewFlick
            anchors.fill: parent
            clip: true
            contentWidth: width
            contentHeight: previewContainer.height
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            interactive: true

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            Item {
                id: previewContainer
                width: previewFlick.width
                height: Math.max(preview.paddedHeight, previewFlick.height)

                Rectangle {
                    anchors.fill: parent
                    color: theme.document
                }

                TextEdit {
                    id: preview
                    readonly property real paddedHeight: contentHeight + topPadding + bottomPadding
                    width: parent.width
                    height: paddedHeight
                    readOnly: true
                    selectByMouse: true
                    textFormat: TextEdit.RichText
                    text: appController.documentPreviewHtml
                    wrapMode: TextEdit.Wrap
                    color: theme.text
                    font.family: "Segoe UI"
                    font.pixelSize: 16 * window.fontScale
                    leftPadding: 36
                    rightPadding: 36
                    topPadding: 24
                    bottomPadding: 40
                    onLinkActivated: function(link) { appController.openLink(link) }
                }
            }
        }

        // Viewport overlay: TextEdit steals wheel events, so forward them here.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            hoverEnabled: false
            onWheel: function(wheel) {
                const dy = wheel.angleDelta.y !== 0 ? wheel.angleDelta.y : wheel.pixelDelta.y
                const maxY = Math.max(0, previewFlick.contentHeight - previewFlick.height)
                previewFlick.contentY = Math.max(0, Math.min(maxY, previewFlick.contentY - dy))
                wheel.accepted = true
            }
        }
    }

    footer: ToolBar {
        visible: !focusMode
        height: focusMode ? 0 : 30
        background: Rectangle { color: theme.panel; border.color: theme.divider; border.width: 1 }
        contentItem: RowLayout {
            Label { text: appController.currentFile.isEmpty ? qsTr("Local document") : appController.currentFile; color: theme.faintText; font.pixelSize: 11; elide: Text.ElideMiddle; Layout.fillWidth: true; leftPadding: 14 }
            Label { text: focusMode ? qsTr("Focus") : qsTr("%1 blocks").arg(appController.documentBlocks.length); color: theme.faintText; font.pixelSize: 11 }
            Label { text: qsTr("%1%").arg(Math.round(window.fontScale * 100)); color: theme.faintText; font.pixelSize: 11 }
            Label { text: qsTr("Marknote %1").arg(appController.version); color: theme.faintText; font.pixelSize: 11; rightPadding: 14 }
        }
    }

    Popup {
        id: recoveryDialog
        visible: appController.recoveryAvailable
        anchors.centerIn: Overlay.overlay
        width: 430
        padding: 20
        modal: true
        closePolicy: Popup.NoAutoClose
        background: Rectangle { color: theme.elevated; border.color: theme.border; radius: theme.radius }
        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            Label { text: qsTr("Recover unsaved document?"); font.pixelSize: 17; font.weight: Font.DemiBold; color: theme.text }
            Label { text: qsTr("Marknote found a recovery copy from your last editing session."); color: theme.mutedText; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button { text: qsTr("Discard"); onClicked: { appController.discardRecovery(); recoveryDialog.close() } }
                Button { text: qsTr("Recover"); highlighted: true; onClicked: { appController.recoverDocument(); recoveryDialog.close() } }
            }
        }
    }

    Popup {
        id: findPopup
        visible: findVisible
        x: parent.width - width - 18
        y: toolbar.height + 8
        width: 390
        padding: 12
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onClosed: findVisible = false
        background: Rectangle { color: theme.elevated; border.color: theme.border; radius: theme.radius }
        onOpened: findField.forceActiveFocus()
        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            RowLayout {
                Layout.fillWidth: true
                TextField { id: findField; placeholderText: qsTr("Find in document"); Layout.fillWidth: true; onAccepted: editorPane.findNext(text) }
                Button { text: qsTr("Next"); onClicked: editorPane.findNext(findField.text) }
            }
            RowLayout {
                Layout.fillWidth: true
                TextField { id: replaceField; placeholderText: qsTr("Replace with"); Layout.fillWidth: true }
                Button { text: qsTr("Replace"); onClicked: editorPane.replaceCurrent(findField.text, replaceField.text) }
                Button { text: qsTr("All"); onClicked: editorPane.replaceAll(findField.text, replaceField.text) }
            }
        }
        Component.onCompleted: findField.forceActiveFocus()
    }

    Connections {
        target: appController
        function onNotificationRequested(message) { toast.text = message; toast.open() }
        function onImagePrepared(relativePath) {
            if (viewMode === 3 && blockPane.activeBlock) {
                appController.insertBlockAfter(blockPane.activeBlock.id, "paragraph")
            }
            editorPane.insertSnippet("![image](" + relativePath + ")")
        }
    }

    Popup {
        id: commandPalette
        visible: commandPaletteVisible
        anchors.centerIn: Overlay.overlay
        width: Math.min(parent.width - 48, 560)
        padding: 12
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onClosed: commandPaletteVisible = false
        onOpened: {
            commandField.text = ""
            refreshCommandList()
            commandField.forceActiveFocus()
        }
        background: Rectangle { color: theme.elevated; border.color: theme.border; radius: theme.radius }

        function refreshCommandList() {
            filteredCommands.clear()
            const query = commandField.text.trim().toLowerCase()
            for (let i = 0; i < commandItems.length; ++i) {
                const item = commandItems[i]
                if (query.length === 0
                        || item.title.toLowerCase().indexOf(query) >= 0
                        || item.id.indexOf(query) >= 0) {
                    filteredCommands.append({
                        commandId: item.id,
                        title: item.title,
                        shortcut: item.shortcut
                    })
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8
            TextField {
                id: commandField
                Layout.fillWidth: true
                placeholderText: qsTr("Type a command…")
                onTextChanged: commandPalette.refreshCommandList()
                onAccepted: {
                    if (filteredCommands.count > 0) {
                        runCommand(filteredCommands.get(0).commandId)
                    }
                }
            }
            ListModel { id: filteredCommands }
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, 320)
                clip: true
                model: filteredCommands
                delegate: ItemDelegate {
                    width: ListView.view.width
                    height: 36
                    text: title + (shortcut ? "  ·  " + shortcut : "")
                    onClicked: runCommand(commandId)
                }
            }
        }
    }

    Popup {
        id: toast
        property alias text: toastLabel.text
        anchors.centerIn: Overlay.overlay
        padding: 11
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        Timer { interval: 2200; running: toast.visible; onTriggered: toast.close() }
        background: Rectangle { color: "#242426"; radius: theme.smallRadius }
        Label { id: toastLabel; color: "#ffffff" }
    }
}
