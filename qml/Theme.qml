import QtQuick

QtObject {
    // ─── Mode Toggle ───
    property bool darkMode: true

    // ─── Elevation ───
    readonly property color bgBase:     darkMode ? "#1e1e2e" : "#eff1f5"
    readonly property color bgSurface:  darkMode ? "#313244" : "#e6e9ef"
    readonly property color bgSurface2: darkMode ? "#3b3d52" : "#dce0e8"
    readonly property color bgOverlay:  darkMode ? "#45475a" : "#ccd0da"
    readonly property color bgHighest:  darkMode ? "#585b70" : "#bcc0cc"

    // ─── Text ───
    readonly property color textPrimary:   darkMode ? "#cdd6f4" : "#4c4f69"
    readonly property color textSecondary: darkMode ? "#a6adc8" : "#6c6f85"
    readonly property color textMuted:     darkMode ? "#7f849c" : "#9ca0b0"

    // ─── Accent ───
    readonly property color accent:          darkMode ? "#89b4fa" : "#1e66f5"
    readonly property color accentHover:     darkMode ? "#a6c8ff" : "#4580f7"
    readonly property color accentSecondary: darkMode ? "#f5c2e7" : "#ea76cb"
    readonly property color accentWarm:      darkMode ? "#f9e2af" : "#df8e1d"
    readonly property color accentGreen:     darkMode ? "#a6e3a1" : "#40a02b"
    readonly property color accentRed:       darkMode ? "#f38ba8" : "#d20f39"

    // ─── Border ───
    readonly property color border:      darkMode ? "#585b70" : "#bcc0cc"
    readonly property color borderSubtle: darkMode ? "#45475a" : "#ccd0da"

    // ─── Spacing (4pt grid) ───
    readonly property int sp2:  2
    readonly property int sp4:  4
    readonly property int sp8:  8
    readonly property int sp12: 12
    readonly property int sp16: 16
    readonly property int sp20: 20
    readonly property int sp24: 24
    readonly property int sp32: 32
    readonly property int sp40: 40
    readonly property int sp48: 48

    // ─── Typography ───
    readonly property int fontCaption:  11
    readonly property int fontSmall:    12
    readonly property int fontBody:     14
    readonly property int fontSubhead:  16
    readonly property int fontTitle:    20
    readonly property int fontDisplay:  28
    readonly property int fontHero:     48

    // ─── Radius ───
    readonly property int radiusSmall:  4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge:  12
    readonly property int radiusPill:   99

    // ─── Animation ───
    readonly property int animFast:     100
    readonly property int animNormal:   180
    readonly property int animSlow:     300

    // ─── Icons ───
    readonly property string iconFolder:    "▸"
    readonly property string iconFolderOpen:"▾"
    readonly property string iconFile:      "◇"
    readonly property string iconMarkdown:  "◆"
    readonly property string iconSettings:  "⚙"
    readonly property string iconRefresh:   "⟳"
    readonly property string iconSearch:    "⌕"
    readonly property string iconBack:      "←"
    readonly property string iconLink:      "⤴"
    readonly property string iconHex:       "⬡"
    readonly property string iconPhoto:     "▣"
    readonly property string iconChat:      "▤"
    readonly property string iconSnap:      "◫"
    readonly property string iconSun:       "☀"
    readonly property string iconMoon:      "☽"
}
