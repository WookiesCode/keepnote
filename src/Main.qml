import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs as Dialogs
import QtQuick.Window

ApplicationWindow {
    id: window
    width: 740
    height: 560
    minimumWidth: 340
    minimumHeight: 240
    visible: true
    title: qsTr("Keepnote")
    color: "#151515"
    Material.theme: Material.Dark
    Material.accent: "#FFD60A"
    property int pendingCloseIndex: -1
    property int viewMode: 0 // 0: editor, 1: split, 2: preview
    // macOS draws its own title bar and owns the global menu bar.
    readonly property bool isMac: Qt.platform.os === "osx" || Qt.platform.os === "macos"
    readonly property bool isWindows: Qt.platform.os === "windows"
    // Windows and macOS decorate the window natively; the in-app title bar is for Linux tiling WMs.
    readonly property bool customTitleBar: !isMac && !isWindows
    menuBar: isMac ? mainMenu : null

    onViewModeChanged: {
        if (viewMode === 2)
            preview.forceActiveFocus()
        else
            editor.forceActiveFocus()
    }

    Connections {
        target: backend
        function onLanguageChanged() {
            if (backend.language !== "markdown" && window.viewMode !== 0)
                window.viewMode = 0
        }
    }

    function requestCloseTab(index) {
        pendingCloseIndex = index
        deleteDialog.open()
    }

    function requestRenameTab(index) {
        renameDialog.tabIndex = index
        renameDialog.open()
    }

    onClosing: function(close) {
        if (!backend.flush())
            close.accepted = false
    }

    Action { id: newTabAction; text: qsTr("New Tab"); shortcut: "Ctrl+N"; onTriggered: backend.addTab() }
    Action { id: openAction; text: qsTr("Open File…"); shortcut: "Ctrl+O"; onTriggered: openFileDialog.open() }
    Action { id: saveAction; text: qsTr("Save File"); shortcut: "Ctrl+S"; onTriggered: backend.saveCurrent() }
    Action { id: saveAsAction; text: qsTr("Save File As…"); shortcut: "Ctrl+Shift+S"; onTriggered: backend.saveCurrentAsDialog() }
    Action { id: closeTabAction; text: qsTr("Close Tab…"); shortcut: "Ctrl+W"; onTriggered: window.requestCloseTab(backend.currentIndex) }
    Action { id: quitAction; text: qsTr("Quit"); shortcut: "Ctrl+Q"; onTriggered: window.close() }
    Shortcut {
        sequence: "Ctrl+T"
        context: Qt.ApplicationShortcut
        onActivated: backend.addTab()
    }
    Shortcut {
        sequence: "F2"
        context: Qt.ApplicationShortcut
        onActivated: window.requestRenameTab(backend.currentIndex)
    }
    Shortcut {
        sequence: window.isMac ? "Meta+Tab" : "Ctrl+Tab"
        context: Qt.ApplicationShortcut
        onActivated: backend.selectTab((backend.currentIndex + 1) % backend.tabs.length)
    }
    Shortcut {
        sequence: window.isMac ? "Meta+Shift+Tab" : "Ctrl+Shift+Tab"
        context: Qt.ApplicationShortcut
        onActivated: backend.selectTab((backend.currentIndex + backend.tabs.length - 1) % backend.tabs.length)
    }

    Dialog {
        id: deleteDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("Close tab?")
        standardButtons: Dialog.Cancel | Dialog.Ok
        onAccepted: backend.closeTab(window.pendingCloseIndex)
        Label {
            text: qsTr("Closing removes this tab's draft. Any saved file stays on disk.")
            wrapMode: Text.WordWrap
        }
    }

    Dialog {
        id: renameDialog
        property int tabIndex: -1
        anchors.centerIn: parent
        width: Math.min(420, window.width - 32)
        modal: true
        title: qsTr("Name tab")
        background: Rectangle { color: "#303030"; radius: 8 }
        standardButtons: Dialog.Cancel | Dialog.Ok
        onOpened: {
            nameField.text = backend.tabs[tabIndex].name
            nameField.forceActiveFocus()
            nameField.selectAll()
        }
        onAccepted: backend.renameTab(tabIndex, nameField.text)

        Column {
            width: renameDialog.availableWidth
            spacing: 8
            Label {
                width: parent.width
                text: qsTr("Leave blank to use the first line")
                wrapMode: Text.WordWrap
            }
            TextField {
                id: nameField
                width: parent.width
                maximumLength: 80
                onAccepted: renameDialog.accept()
            }
        }
    }

    Connections {
        target: backend
        function onSaveFileRequested(suggestedUrl) {
            saveFileDialog.selectedFile = suggestedUrl
            saveFileDialog.open()
        }
    }

    Dialogs.FileDialog {
        id: openFileDialog
        title: qsTr("Open text file")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [qsTr("Supported text files (*.txt *.md *.markdown *.log *.json *.jsonl *.yaml *.yml *.toml *.py *.pyw *.js *.mjs *.cjs *.jsx *.lua *.sh *.bash *.zsh *.ts *.tsx *.mts *.cts)"), qsTr("All files (*)")]
        onAccepted: backend.openFile(selectedFile)
    }

    Dialogs.FileDialog {
        id: saveFileDialog
        title: qsTr("Save current tab to file")
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: [qsTr("All files (*)"), qsTr("Markdown files (*.md)"), qsTr("Text files (*.txt)")]
        onAccepted: backend.saveCurrentTo(selectedFile)
    }

    Column {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: titleBar
            width: parent.width
            height: window.customTitleBar ? 48 : 0
            visible: window.customTitleBar
            color: "#202020"
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: window.startSystemMove()
                onDoubleClicked: window.visibility === Window.Maximized
                    ? window.showNormal() : window.showMaximized()
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 20
                text: qsTr("Keepnote")
                font.bold: true
                font.pixelSize: 16
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                ToolButton {
                    width: 42
                    height: 36
                    text: "−"
                    font.pixelSize: 20
                    Accessible.name: qsTr("Minimize window")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Minimize")
                    onClicked: window.showMinimized()
                }
                ToolButton {
                    width: 42
                    height: 36
                    text: window.visibility === Window.Maximized ? "❐" : "□"
                    font.pixelSize: 18
                    Accessible.name: window.visibility === Window.Maximized
                        ? qsTr("Restore window") : qsTr("Maximize window")
                    ToolTip.visible: hovered
                    ToolTip.text: window.visibility === Window.Maximized
                        ? qsTr("Restore") : qsTr("Maximize")
                    onClicked: window.visibility === Window.Maximized
                        ? window.showNormal() : window.showMaximized()
                }
                ToolButton {
                    width: 42
                    height: 36
                    text: "×"
                    font.pixelSize: 20
                    Accessible.name: qsTr("Close window")
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Close window")
                    onClicked: window.close()
                }
            }
        }

        MenuBar {
            id: mainMenu
            width: parent.width
            Menu {
                title: qsTr("File")
                width: 230
                Material.theme: Material.Dark
                background: Rectangle { color: "#303030"; border.color: "#4a4a4a"; radius: 4 }
                MenuItem { action: newTabAction }
                MenuItem { action: openAction }
                MenuSeparator {}
                MenuItem { action: saveAction }
                MenuItem { action: saveAsAction }
                MenuSeparator {}
                MenuItem { action: closeTabAction }
                MenuItem { action: quitAction }
            }
            Menu {
                title: qsTr("Edit")
                width: 190
                Material.theme: Material.Dark
                background: Rectangle { color: "#303030"; border.color: "#4a4a4a"; radius: 4 }
                MenuItem { text: qsTr("Undo"); enabled: editor.canUndo; onTriggered: editor.undo() }
                MenuItem { text: qsTr("Redo"); enabled: editor.canRedo; onTriggered: editor.redo() }
                MenuSeparator {}
                MenuItem { text: qsTr("Cut"); enabled: editor.selectedText.length > 0; onTriggered: editor.cut() }
                MenuItem { text: qsTr("Copy"); enabled: editor.selectedText.length > 0; onTriggered: editor.copy() }
                MenuItem { text: qsTr("Paste"); onTriggered: editor.paste() }
                MenuItem { text: qsTr("Select All"); onTriggered: editor.selectAll() }
            }
            Menu {
                title: qsTr("View")
                width: 170
                Material.theme: Material.Dark
                background: Rectangle { color: "#303030"; border.color: "#4a4a4a"; radius: 4 }
                MenuItem { text: qsTr("Editor"); checkable: true; checked: window.viewMode === 0; onTriggered: window.viewMode = 0 }
                MenuItem { text: qsTr("Split"); enabled: backend.language === "markdown"; checkable: true; checked: window.viewMode === 1; onTriggered: window.viewMode = 1 }
                MenuItem { text: qsTr("Preview"); enabled: backend.language === "markdown"; checkable: true; checked: window.viewMode === 2; onTriggered: window.viewMode = 2 }
            }
            Menu {
                title: qsTr("Language")
                width: 190
                Material.theme: Material.Dark
                background: Rectangle { color: "#303030"; border.color: "#4a4a4a"; radius: 4 }
                MenuItem { height: 36; text: qsTr("Plain Text"); checkable: true; checked: backend.language === "plain"; onTriggered: backend.setLanguage("plain") }
                MenuItem { height: 36; text: qsTr("Markdown"); checkable: true; checked: backend.language === "markdown"; onTriggered: backend.setLanguage("markdown") }
                MenuItem { height: 36; text: qsTr("Log"); checkable: true; checked: backend.language === "log"; onTriggered: backend.setLanguage("log") }
                Menu {
                    title: qsTr("Data formats")
                    width: 190
                    background: Rectangle { color: "#303030"; border.color: "#4a4a4a"; radius: 4 }
                    MenuItem { height: 36; text: qsTr("JSON"); checkable: true; checked: backend.language === "json"; onTriggered: backend.setLanguage("json") }
                    MenuItem { height: 36; text: qsTr("YAML"); checkable: true; checked: backend.language === "yaml"; onTriggered: backend.setLanguage("yaml") }
                    MenuItem { height: 36; text: qsTr("TOML"); checkable: true; checked: backend.language === "toml"; onTriggered: backend.setLanguage("toml") }
                }
                Menu {
                    title: qsTr("Code")
                    width: 190
                    background: Rectangle { color: "#303030"; border.color: "#4a4a4a"; radius: 4 }
                    MenuItem { height: 36; text: qsTr("Python"); checkable: true; checked: backend.language === "python"; onTriggered: backend.setLanguage("python") }
                    MenuItem { height: 36; text: qsTr("JavaScript"); checkable: true; checked: backend.language === "javascript"; onTriggered: backend.setLanguage("javascript") }
                    MenuItem { height: 36; text: qsTr("Lua"); checkable: true; checked: backend.language === "lua"; onTriggered: backend.setLanguage("lua") }
                    MenuItem { height: 36; text: qsTr("Bash / Shell"); checkable: true; checked: backend.language === "shell"; onTriggered: backend.setLanguage("shell") }
                    MenuItem { height: 36; text: qsTr("TypeScript"); checkable: true; checked: backend.language === "typescript"; onTriggered: backend.setLanguage("typescript") }
                }
            }
        }

        Rectangle {
            id: tabStrip
            width: parent.width
            height: 44
            color: "#202020"

            Flickable {
                anchors.fill: parent
                contentWidth: tabRow.width
                contentHeight: height
                clip: true
                flickableDirection: Flickable.HorizontalFlick

                Row {
                    id: tabRow
                    height: parent.height
                    spacing: 2

                    Repeater {
                        model: backend.tabs
                        delegate: Rectangle {
                            required property int index
                            required property var modelData
                            width: 190
                            height: tabRow.height
                            color: backend.currentIndex === index ? "#151515" : "#292929"

                            Label {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.right: renameButton.left
                                anchors.rightMargin: 4
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.title
                                elide: Text.ElideRight
                                font.pixelSize: 13
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: backend.selectTab(index)
                                onDoubleClicked: window.requestRenameTab(index)
                            }
                            ToolButton {
                                id: renameButton
                                anchors.right: closeTab.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 28
                                height: 32
                                text: "✎"
                                Accessible.name: qsTr("Name tab")
                                onClicked: window.requestRenameTab(index)
                            }
                            ToolButton {
                                id: closeTab
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: 28
                                height: 32
                                text: "×"
                                Accessible.name: qsTr("Close tab")
                                onClicked: window.requestCloseTab(index)
                            }
                        }
                    }

                    ToolButton {
                        width: 42
                        height: tabRow.height
                        text: "+"
                        font.pixelSize: 20
                        onClicked: backend.addTab()
                    }
                }
            }
        }

        Item {
            width: parent.width
            height: parent.height - titleBar.height - (window.isMac ? 0 : mainMenu.height) - tabStrip.height - statusBar.height

            Row {
                anchors.fill: parent
                spacing: 0

                Item {
                    id: editorPane
                    width: window.viewMode === 2 ? 0
                        : window.viewMode === 1 ? Math.floor(parent.width / 2) : parent.width
                    height: parent.height
                    visible: window.viewMode !== 2

                    TextArea {
                        id: editor
                        anchors.fill: parent
                        leftPadding: 20
                        rightPadding: 20
                        topPadding: 18
                        bottomPadding: 18
                        text: backend.text
                        textFormat: TextEdit.PlainText
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        font.family: window.isMac ? "Menlo" : window.isWindows ? "Consolas" : "monospace"
                        font.pixelSize: 16
                        background: Rectangle { color: "#151515" }
                        onTextChanged: backend.setText(text)
                        Component.onCompleted: {
                            backend.attachEditor(textDocument)
                            forceActiveFocus()
                        }
                    }
                    Label {
                        anchors.left: editor.left
                        anchors.leftMargin: editor.leftPadding
                        anchors.right: editor.right
                        anchors.rightMargin: editor.rightPadding
                        anchors.top: editor.top
                        anchors.topMargin: editor.topPadding
                        visible: editor.text.length === 0
                        text: qsTr("Write Markdown or plain text. It will be here when you return.")
                        color: "#777"
                        font: editor.font
                        wrapMode: Text.WordWrap
                    }
                }

                Rectangle {
                    width: window.viewMode === 1 ? 1 : 0
                    height: parent.height
                    color: "#383838"
                    visible: window.viewMode === 1
                }

                Rectangle {
                    id: previewPane
                    width: window.viewMode === 0 ? 0
                        : window.viewMode === 1 ? parent.width - editorPane.width - 1 : parent.width
                    height: parent.height
                    visible: window.viewMode !== 0
                    color: "#1c1c1c"

                    ScrollView {
                        anchors.fill: parent
                        TextArea {
                            id: preview
                            readOnly: true
                            text: backend.text
                            textFormat: TextEdit.MarkdownText
                            wrapMode: TextEdit.Wrap
                            leftPadding: 20
                            rightPadding: 20
                            topPadding: 18
                            bottomPadding: 18
                            color: "#e9e9e9"
                            font.family: "sans-serif"
                            font.pixelSize: 16
                            background: Rectangle { color: "#1c1c1c" }
                            onLinkActivated: function(link) { Qt.openUrlExternally(link) }
                        }
                    }
                }
            }
        }
        Connections {
            target: backend
            function onTextChanged() {
                if (editor.text !== backend.text)
                    editor.text = backend.text
            }
        }

        Rectangle {
            id: statusBar
            width: parent.width
            height: 32
            color: "#202020"
            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 20
                text: qsTr("Language: %1").arg(backend.language)
                color: "#aaa"
                font.pixelSize: 12
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 20
                text: backend.status || qsTr("Draft saved automatically")
                color: backend.hasError ? "#ff8585" : "#aaa"
                font.pixelSize: 12
            }
        }
    }
}
