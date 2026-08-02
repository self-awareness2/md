import QtQuick

QtObject {
    property bool dark: false
    readonly property color window: dark ? "#1d1d1f" : "#f5f5f7"
    readonly property color panel: dark ? "#2a2a2c" : "#fbfbfd"
    readonly property color elevated: dark ? "#303033" : "#ffffff"
    readonly property color document: dark ? "#242426" : "#ffffff"
    readonly property color text: dark ? "#f5f5f7" : "#1d1d1f"
    readonly property color mutedText: dark ? "#a1a1a6" : "#6e6e73"
    readonly property color faintText: dark ? "#7d7d82" : "#98989d"
    readonly property color border: dark ? "#444448" : "#dedee3"
    readonly property color divider: dark ? "#3a3a3c" : "#e7e7eb"
    readonly property color accent: "#0a84ff"
    readonly property color accentSoft: dark ? "#173b61" : "#e5f1ff"
    readonly property color selection: dark ? "#315b89" : "#b9d9ff"
    readonly property color codeBackground: dark ? "#303033" : "#f6f6f8"
    readonly property int radius: 10
    readonly property int smallRadius: 7
    readonly property int spacing: 8
    readonly property int contentWidth: 860
}
