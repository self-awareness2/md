import QtQuick

QtObject {
    property bool dark: false
    // default | cream | sepia | mint | night
    property string paper: "default"

    readonly property bool usingPaper: paper !== "default"

    readonly property color window: dark ? "#1d1d1f" : "#f5f5f7"
    readonly property color panel: dark ? "#2a2a2c" : "#fbfbfd"
    readonly property color elevated: dark ? "#303033" : "#ffffff"

    readonly property color document: {
        switch (paper) {
        case "cream": return "#f4efe4"
        case "sepia": return "#efe2c8"
        case "mint": return "#e7efea"
        case "night": return "#1b1e24"
        default: return dark ? "#242426" : "#ffffff"
        }
    }

    readonly property color text: {
        switch (paper) {
        case "cream": return "#3f382c"
        case "sepia": return "#4d3c28"
        case "mint": return "#2a352e"
        case "night": return "#e6eaf0"
        default: return dark ? "#f5f5f7" : "#1d1d1f"
        }
    }

    readonly property color mutedText: {
        switch (paper) {
        case "cream": return "#6e6454"
        case "sepia": return "#7a6548"
        case "mint": return "#5a6a5f"
        case "night": return "#9aa3ad"
        default: return dark ? "#a1a1a6" : "#6e6e73"
        }
    }

    readonly property color faintText: {
        switch (paper) {
        case "cream": return "#95897a"
        case "sepia": return "#9a8566"
        case "mint": return "#7f9086"
        case "night": return "#7d8590"
        default: return dark ? "#7d7d82" : "#98989d"
        }
    }

    readonly property color border: dark ? "#444448" : "#dedee3"
    readonly property color divider: dark ? "#3a3a3c" : "#e7e7eb"
    readonly property color accent: "#0a84ff"
    readonly property color accentSoft: dark ? "#173b61" : "#e5f1ff"
    readonly property color selection: dark ? "#315b89" : "#b9d9ff"

    readonly property color codeBackground: {
        switch (paper) {
        case "cream": return "#ebe4d4"
        case "sepia": return "#e4d5b5"
        case "mint": return "#dce6df"
        case "night": return "#262a32"
        default: return dark ? "#303033" : "#f6f6f8"
        }
    }

    readonly property color paperSwatch: document

    readonly property int radius: 10
    readonly property int smallRadius: 7
    readonly property int spacing: 8
    readonly property int contentWidth: 860
}
