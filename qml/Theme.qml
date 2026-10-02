pragma Singleton
import QtCore
import QtQuick

// 全局配色与尺寸。颜色取自 Tailwind Slate 色板（沿用旧界面 ThemeManager 的色值）。
// 默认跟随系统深浅色；用户手动切换后以手动为准。
QtObject {
    id: theme

    // 0 跟随系统，1 浅色，2 深色
    property int mode: 0
    readonly property bool dark: mode === 2 || (mode === 0 && Qt.styleHints.colorScheme !== Qt.ColorScheme.Light)

    function cycleMode() { mode = (mode + 1) % 3 }

    property Settings settings: Settings {
        category: "ui"
        property alias themeMode: theme.mode
    }

    // ---- 颜色 ----
    readonly property color bg:          dark ? "#0f172a" : "#f1f5f9"
    readonly property color surface:     dark ? "#1e293b" : "#ffffff"
    readonly property color surfaceAlt:  dark ? "#273449" : "#f8fafc"
    readonly property color border:      dark ? "#334155" : "#e2e8f0"
    readonly property color text:        dark ? "#f1f5f9" : "#0f172a"
    readonly property color textMuted:   dark ? "#94a3b8" : "#475569"
    readonly property color textFaint:   dark ? "#64748b" : "#94a3b8"
    readonly property color accent:      dark ? "#38bdf8" : "#0284c7"
    readonly property color accentBg:    dark ? "#0c4a6e" : "#e0f2fe"
    readonly property color codeBg:      dark ? "#0b1220" : "#f8fafc"

    // 语义色：每种块一个，文字色 + 浅底色
    readonly property color why:         dark ? "#a78bfa" : "#6d28d9"
    readonly property color whyBg:       dark ? "#2e1065" : "#f5f3ff"
    readonly property color pitfall:     dark ? "#fb923c" : "#c2410c"
    readonly property color pitfallBg:   dark ? "#431407" : "#fff7ed"
    readonly property color tryIt:       dark ? "#4ade80" : "#15803d"
    readonly property color tryItBg:     dark ? "#052e16" : "#f0fdf4"
    readonly property color inSystem:    dark ? "#f472b6" : "#be185d"
    readonly property color inSystemBg:  dark ? "#500724" : "#fdf2f8"
    readonly property color mainLane:    dark ? "#38bdf8" : "#0369a1"
    readonly property color workerLane:  dark ? "#fbbf24" : "#b45309"
    readonly property color danger:      dark ? "#f87171" : "#b91c1c"

    // ---- 尺寸 ----
    readonly property int radius: 10
    readonly property int gap: 16
    readonly property int fontBody: 15
    readonly property int fontSmall: 13
    readonly property int fontTitle: 26
    readonly property int fontHeading: 18
    readonly property int readingWidth: 860   // 正文最大宽度，太宽不好读
}
