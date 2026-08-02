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

    Component.onCompleted: { rebuildOutline(); rebuildBlocks() }

    property int viewMode: 2 // 0: source, 1: preview, 2: split, 3: block
    property bool sidebarVisible: true
    property bool darkMode: false
    property bool findVisible: false
    property real fontScale: 1.0

    function zoomIn() { fontScale = Math.min(1.6, fontScale + 0.1) }
    function zoomOut() { fontScale = Math.max(0.7, fontScale - 0.1) }
    function resetZoom() { fontScale = 1.0 }

    onViewModeChanged: if (viewMode === 3) rebuildBlocks()

    ListModel { id: outlineModel }
    ListModel { id: blockModel }
    function rebuildOutline() {
        outlineModel.clear()
        const lines = appController.documentText.split("\n")
        let offset = 0
        for (let i = 0; i < lines.length; ++i) {
            const match = /^(#{1,6})\s+(.+?)\s*$/.exec(lines[i])
            if (match) {
                outlineModel.append({title: match[2], level: match[1].length, position: offset})
            }
            offset += lines[i].length + 1
        }
    }

    function rebuildBlocks() {
        blockModel.clear()
        if (!appController.hasDocument) {
            return
        }
        const source = appController.documentText
        const chunks = source.split(/\n\s*\n/)
        for (let i = 0; i < chunks.length; ++i) {
            const block = chunks[i]
            if (block.length === 0) {
                continue
            }
            let kind = "paragraph"
            let level = 0
            if (/^#{1,6}\s/.test(block)) {
                kind = "heading"
                level = /^#+/.exec(block)[0].length
            } else if (/^```/.test(block) || /^~~~/.test(block) || /^ {4}/.test(block)) {
                kind = "code"
            } else if (/^>\s?/.test(block)) {
                kind = "quote"
            } else if (/^(?:[-+*]\s|\d+[.)]\s|[-+*]\s+\[[ xX]\]\s)/.test(block)) {
                kind = "list"
            } else if (/^\|.*\|\s*\n\|?\s*:?-{3,}/.test(block)) {
                kind = "table"
            } else if (/^(?:---+|\*\*\*+|___+)$/.test(block.trim())) {
                kind = "rule"
            }
            blockModel.append({source: block, kind: kind, level: level})
        }
    }

    Action { id: newAction; text: qsTr("New Document"); shortcut: StandardKey.New; onTriggered: appController.newDocument() }
    Action { id: openAction; text: qsTr("Open..."); shortcut: StandardKey.Open; onTriggered: appController.openFile() }
    Action { id: saveAction; text: qsTr("Save"); shortcut: StandardKey.Save; onTriggered: appController.save() }
    Action { id: saveAsAction; text: qsTr("Save As..."); shortcut: StandardKey.SaveAs; onTriggered: appController.saveAs() }
    Action { id: exportAction; text: qsTr("Export HTML..."); onTriggered: appController.exportHtmlAs() }
    Action { id: exportPdfAction; text: qsTr("Export PDF..."); onTriggered: appController.exportPdfAs() }
    Action { id: undoAction; text: qsTr("Undo"); shortcut: StandardKey.Undo; onTriggered: viewMode === 3 ? blockPane.undo() : editorPane.undo() }
    Action { id: redoAction; text: qsTr("Redo"); shortcut: StandardKey.Redo; onTriggered: viewMode === 3 ? blockPane.redo() : editorPane.redo() }
    Action { id: findAction; text: qsTr("Find"); shortcut: StandardKey.Find; onTriggered: findVisible = true }
    Action { id: zoomInAction; text: qsTr("Zoom In"); shortcut: "Ctrl+="; onTriggered: zoomIn() }
    Action { id: zoomOutAction; text: qsTr("Zoom Out"); shortcut: "Ctrl+-"; onTriggered: zoomOut() }
    Action { id: resetZoomAction; text: qsTr("Reset Zoom"); shortcut: "Ctrl+0"; onTriggered: resetZoom() }

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            Action { text: newAction.text; shortcut: newAction.shortcut; onTriggered: newAction.trigger() }
            Action { text: openAction.text; shortcut: openAction.shortcut; onTriggered: openAction.trigger() }
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
        }
        Menu {
            title: qsTr("View")
            Action { text: qsTr("Source"); checkable: true; checked: viewMode === 0; onTriggered: viewMode = 0 }
            Action { text: qsTr("Split"); checkable: true; checked: viewMode === 2; onTriggered: viewMode = 2 }
            Action { text: qsTr("Preview"); checkable: true; checked: viewMode === 1; onTriggered: viewMode = 1 }
            Action { text: qsTr("Block Editing"); checkable: true; checked: viewMode === 3; onTriggered: viewMode = 3 }
            MenuSeparator {}
            Action { text: qsTr("Toggle Sidebar"); shortcut: "Ctrl+Shift+B"; onTriggered: sidebarVisible = !sidebarVisible }
            Action { text: qsTr("Toggle Dark Appearance"); checkable: true; checked: darkMode; onTriggered: darkMode = !darkMode }
            MenuSeparator {}
            Action { text: zoomInAction.text; shortcut: zoomInAction.shortcut; onTriggered: zoomInAction.trigger() }
            Action { text: zoomOutAction.text; shortcut: zoomOutAction.shortcut; onTriggered: zoomOutAction.trigger() }
            Action { text: resetZoomAction.text; shortcut: resetZoomAction.shortcut; onTriggered: resetZoomAction.trigger() }
        }
    }

    header: ToolBar {
        id: toolbar
        height: 54
        background: Rectangle { color: theme.panel; border.color: theme.divider; border.width: 1 }
        contentItem: RowLayout {
            spacing: 6
            ToolButton {
                icon.name: sidebarVisible ? "sidebar" : "sidebar-show"
                display: AbstractButton.IconOnly
                onClicked: sidebarVisible = !sidebarVisible
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Toggle sidebar")
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
                ToolButton { text: "`"; onClicked: editorPane.wrapSelection("`", "`"); ToolTip.visible: hovered; ToolTip.text: qsTr("Inline code") }
                ToolButton { text: "H"; onClicked: editorPane.prefixLine("## "); ToolTip.visible: hovered; ToolTip.text: qsTr("Heading") }
                ToolButton { text: "-"; onClicked: editorPane.prefixLine("- "); ToolTip.visible: hovered; ToolTip.text: qsTr("Bullet list") }
                ToolButton { text: "T"; onClicked: editorPane.insertTable(); ToolTip.visible: hovered; ToolTip.text: qsTr("Insert table") }
            }
            ToolButton { visible: viewMode === 3; text: "T"; onClicked: blockPane.insertTable(); ToolTip.visible: hovered; ToolTip.text: qsTr("Insert table block") }
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

        Pane {
            visible: sidebarVisible
            Layout.preferredWidth: 248
            Layout.fillHeight: true
            padding: 16
            background: Rectangle { color: theme.panel; border.color: theme.divider; border.width: 1 }
            ColumnLayout {
                anchors.fill: parent
                spacing: 12
                Label { text: qsTr("WORKSPACE"); color: theme.faintText; font.pixelSize: 11; font.weight: Font.DemiBold; Layout.bottomMargin: 4 }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    radius: theme.smallRadius
                    color: appController.hasDocument ? theme.accentSoft : "transparent"
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 8; spacing: 8
                        Label { text: "*"; color: theme.accent; font.pixelSize: 18; visible: appController.hasDocument }
                        Label {
                            text: appController.hasDocument ? appController.currentFileName : qsTr("No document open")
                            color: appController.hasDocument ? theme.text : theme.mutedText
                            elide: Text.ElideMiddle; Layout.fillWidth: true
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: qsTr("RECENT"); color: theme.faintText; font.pixelSize: 11; font.weight: Font.DemiBold; Layout.fillWidth: true }
                    ToolButton { text: qsTr("Clear"); visible: appController.recentFiles.length > 0; onClicked: appController.clearRecentFiles() }
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(contentHeight, 150)
                    model: appController.recentFiles
                    clip: true
                    delegate: ItemDelegate {
                        width: ListView.view.width
                        height: 28
                        text: modelData
                        font.pixelSize: 11
                        onClicked: appController.openPath(modelData)
                    }
                    visible: count > 0
                }
                Label { text: qsTr("OUTLINE"); color: theme.faintText; font.pixelSize: 11; font.weight: Font.DemiBold; Layout.topMargin: 8 }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(contentHeight, 260)
                    model: outlineModel
                    clip: true
                    delegate: ItemDelegate {
                        width: ListView.view.width
                        height: 30
                        leftPadding: 8 + (level - 1) * 12
                        text: title
                        font.pixelSize: 12
                        onClicked: editorPane.revealPosition(position)
                    }
                    visible: count > 0
                }
                Item { Layout.fillHeight: true }
                Label { text: qsTr("LOCAL DOCUMENT"); color: theme.faintText; font.pixelSize: 11; font.weight: Font.DemiBold }
                Label { text: appController.hasDocument ? appController.currentFile : qsTr("Open a Markdown file to get started"); color: theme.mutedText; font.pixelSize: 11; wrapMode: Text.Wrap; Layout.fillWidth: true }
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
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
                    EditorPane { id: editorPane; visible: viewMode === 0 || viewMode === 2; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1 }
                    Rectangle { visible: viewMode === 2; Layout.preferredWidth: 1; Layout.fillHeight: true; color: theme.divider }
                    PreviewPane { visible: viewMode === 1 || viewMode === 2; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1 }
                    BlockPane { id: blockPane; visible: viewMode === 3; Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 1 }
                }
                Label { text: qsTr("%1 words  /  %2 characters").arg(editorPane.wordCount).arg(appController.documentText.length); color: theme.faintText; font.pixelSize: 11; Layout.alignment: Qt.AlignRight; Layout.rightMargin: 28; Layout.bottomMargin: 10 }
            }

            ColumnLayout {
                anchors.centerIn: parent; width: Math.min(parent.width - 64, 520); spacing: 14; visible: !appController.hasDocument
                Label { text: qsTr("A quiet place for your words."); font.pixelSize: 28; font.weight: Font.DemiBold; color: theme.text; Layout.alignment: Qt.AlignHCenter }
                Label { text: qsTr("Create a document or open an existing Markdown file to begin."); color: theme.mutedText; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; Layout.fillWidth: true }
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
            leftPadding: 34; rightPadding: 34; topPadding: 24; bottomPadding: 40
            background: Rectangle { color: theme.document }
            onTextChanged: if (activeFocus) appController.setDocumentText(text)
            Keys.onPressed: function(event) {
                if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_S) { appController.save(); event.accepted = true }
                else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_B) { editorPane.wrapSelection("**", "**"); event.accepted = true }
                else if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_I) { editorPane.wrapSelection("*", "*"); event.accepted = true }
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
        function undo() {
            const item = activeIndex >= 0 ? listView.itemAtIndex(activeIndex) : null
            if (item && item.editorObject) item.editorObject.undo()
        }
        function redo() {
            const item = activeIndex >= 0 ? listView.itemAtIndex(activeIndex) : null
            if (item && item.editorObject) item.editorObject.redo()
        }
        function syncSource() {
            const pieces = []
            for (let i = 0; i < blockModel.count; ++i) {
                pieces.push(blockModel.get(i).source)
            }
            appController.setDocumentText(pieces.join("\n\n"))
        }
        function commitBlock(blockIndex, value) {
            if (blockIndex < 0 || blockIndex >= blockModel.count) {
                return
            }
            if (blockModel.get(blockIndex).source === value) {
                return
            }
            blockModel.setProperty(blockIndex, "source", value)
            syncSource()
        }
        function addBlock() {
            blockModel.append({source: "", kind: "paragraph", level: 0})
            syncSource()
            listView.currentIndex = blockModel.count - 1
        }
        function insertTable() {
            blockModel.append({source: "| Column 1 | Column 2 |\n| --- | --- |\n|  |  |", kind: "table", level: 0})
            syncSource()
            listView.currentIndex = blockModel.count - 1
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
            model: blockModel
            spacing: 10
            delegate: Item {
                property var editorObject: null
                width: ListView.view.width
                height: Math.max(58, blockEditor.contentHeight + 22)
                TextArea {
                    id: blockEditor
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 40
                    anchors.rightMargin: 40
                    anchors.top: parent.top
                    text: source
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    color: theme.text
                    selectionColor: theme.selection
                    selectedTextColor: theme.text
                    font.family: kind === "code" ? "Consolas" : "Segoe UI"
                    font.pixelSize: (kind === "heading" ? Math.max(18, 30 - level * 2) : 16) * window.fontScale
                    font.bold: kind === "heading"
                    leftPadding: 12
                    rightPadding: 12
                    topPadding: 10
                    bottomPadding: 10
                    background: Rectangle {
                        color: kind === "code" ? theme.codeBackground : kind === "table" ? theme.accentSoft : theme.document
                        radius: theme.smallRadius
                        border.color: blockEditor.activeFocus ? theme.accent : theme.divider
                        border.width: blockEditor.activeFocus ? 2 : 1
                    }
                    onTextChanged: if (activeFocus) blockPane.commitBlock(index, text)
                    onActiveFocusChanged: if (activeFocus) blockPane.activeIndex = index
                    Keys.onPressed: function(event) {
                        if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_Enter) {
                            blockPane.addBlock()
                            event.accepted = true
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

    component PreviewPane: ScrollView {
        clip: true
        Rectangle {
            color: theme.document
            implicitWidth: Math.max(parent ? parent.width : 0, 420)
            implicitHeight: preview.contentHeight + 64
            TextEdit {
                id: preview
                anchors.left: parent.left; anchors.right: parent.right
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.RichText
                text: appController.documentPreviewHtml
                wrapMode: TextEdit.Wrap
                color: theme.text
                font.family: "Segoe UI"
                font.pixelSize: 16 * window.fontScale
                leftPadding: 36; rightPadding: 36; topPadding: 24; bottomPadding: 40
                onLinkActivated: function(link) { appController.openLink(link) }
            }
        }
    }

    footer: ToolBar {
        height: 30
        background: Rectangle { color: theme.panel; border.color: theme.divider; border.width: 1 }
        contentItem: RowLayout {
            Label { text: appController.currentFile.isEmpty ? qsTr("Local document") : appController.currentFile; color: theme.faintText; font.pixelSize: 11; elide: Text.ElideMiddle; Layout.fillWidth: true; leftPadding: 14 }
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
        function onCurrentFileChanged() { if (viewMode === 3) rebuildBlocks() }
        function onDocumentTextChanged() {
            rebuildOutline()
            if (viewMode !== 3) {
                rebuildBlocks()
            }
        }
        function onImagePrepared(relativePath) { editorPane.insertSnippet("![image](" + relativePath + ")") }
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
