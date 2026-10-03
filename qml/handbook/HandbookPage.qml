import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 手册：左边目录（卷 → 章 → 节），右边正文。
// 目录在 handbook/Index.qml，正文是 handbook/ 下的 QML 文件，运行时按路径加载。
Item {
    id: page

    property var index: null           // Index.qml 实例
    property int volumeIndex: 0
    property string currentFile: ""
    property string currentVolume: ""
    property string currentChapter: ""
    property string loadError: ""

    readonly property var volume: index && index.volumes.length > volumeIndex ? index.volumes[volumeIndex] : null

    Settings {
        category: "handbook"
        property alias lastFile: page.currentFile
        property alias lastVolume: page.volumeIndex
    }

    function loadIndex() {
        SourceProvider.clearCache()
        const comp = Qt.createComponent(SourceProvider.contentUrl("handbook/Index.qml"))
        if (comp.status !== Component.Ready) {
            loadError = comp.errorString()
            return
        }
        if (index)
            index.destroy()
        index = comp.createObject(page)
        SourceProvider.watch("handbook/Index.qml")
        if (volumeIndex >= index.volumes.length)
            volumeIndex = 0
    }

    function open(file, volumeTitle, chapterTitle) {
        currentFile = file
        currentVolume = volumeTitle
        currentChapter = chapterTitle
        reload()
    }

    function reload() {
        if (!currentFile)
            return
        loadError = ""
        content.source = ""
        // 旧的正文对象要等回到事件循环才真正销毁；在那之前清缓存，引擎还认为这个组件在用，
        // 会继续用缓存里的旧版本。所以等一轮事件循环再清缓存、重新加载
        Qt.callLater(() => {
            SourceProvider.clearCache()
            content.source = SourceProvider.contentUrl(currentFile)
            SourceProvider.watch(currentFile)
        })
    }

    // 已写 / 总数
    function progress(vol) {
        let done = 0, total = 0
        for (const ch of vol.chapters)
            for (const e of ch.entries) { total++; if (e.file) done++ }
        return [done, total]
    }

    // 根据文件路径在目录里找回所属的卷和章（上次打开的那一节只保存了路径）
    function locate(file) {
        if (!index) return false
        for (let v = 0; v < index.volumes.length; v++)
            for (const ch of index.volumes[v].chapters)
                for (const e of ch.entries)
                    if (e.file === file) {
                        volumeIndex = v
                        currentVolume = index.volumes[v].title
                        currentChapter = ch.title
                        return true
                    }
        return false
    }

    Component.onCompleted: {
        loadIndex()
        if (currentFile && locate(currentFile))
            reload()
        else
            currentFile = ""
    }

    Connections {
        target: SourceProvider
        function onFileChanged(path) {
            if (path === "handbook/Index.qml")
                page.loadIndex()
            else if (path === page.currentFile)
                page.reload()
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ---------------- 目录 ----------------
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 300
            color: Theme.surface

            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("手册")
                        font.pixelSize: Theme.fontHeading + 2
                        font.weight: Font.Bold
                        color: Theme.text
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        visible: SourceProvider.devMode
                        text: qsTr("开发模式 · 保存即刷新")
                        font.pixelSize: 11
                        color: Theme.tryIt
                    }
                }

                // 卷
                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: page.index ? page.index.volumes : []
                        delegate: Rectangle {
                            id: pill
                            required property var modelData
                            required property int index
                            readonly property bool active: page.volumeIndex === index
                            width: pillText.implicitWidth + 22
                            height: 30
                            radius: 15
                            color: active ? Theme.accent : (pillHover.hovered ? Theme.surfaceAlt : "transparent")
                            border.color: active ? Theme.accent : Theme.border
                            Text {
                                id: pillText
                                anchors.centerIn: parent
                                text: pill.modelData.title
                                font.pixelSize: Theme.fontSmall
                                font.weight: pill.active ? Font.DemiBold : Font.Normal
                                color: pill.active ? (Theme.dark ? "#0f172a" : "#ffffff") : Theme.textMuted
                            }
                            HoverHandler { id: pillHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler { onTapped: page.volumeIndex = pill.index }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    visible: page.volume !== null
                    text: {
                        if (!page.volume) return ""
                        const p = page.progress(page.volume)
                        return page.volume.summary + "\n" + qsTr("已写 %1 / %2 节").arg(p[0]).arg(p[1])
                    }
                    wrapMode: Text.Wrap
                    lineHeight: 1.3
                    font.pixelSize: 12
                    color: Theme.textFaint
                }

                // 章与节
                ListView {
                    id: toc
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    model: {
                        const rows = []
                        if (!page.volume) return rows
                        for (const ch of page.volume.chapters) {
                            rows.push({ kind: "chapter", title: ch.title, chapter: ch.title, file: "", from: "" })
                            for (const e of ch.entries)
                                rows.push({ kind: "entry", title: e.title, chapter: ch.title, file: e.file, from: e.from })
                        }
                        return rows
                    }

                    delegate: Item {
                        id: row
                        required property var modelData
                        readonly property bool isChapter: modelData.kind === "chapter"
                        readonly property bool ready: modelData.file.length > 0
                        readonly property bool active: ready && page.currentFile === modelData.file

                        width: toc.width
                        height: isChapter ? 38 : 32

                        Rectangle {
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            radius: 6
                            visible: !row.isChapter
                            color: row.active ? Theme.accentBg : (rowHover.hovered && row.ready ? Theme.surfaceAlt : "transparent")
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.verticalCenterOffset: row.isChapter ? 4 : 0
                            x: row.isChapter ? 2 : 16
                            width: parent.width - x - 44
                            elide: Text.ElideRight
                            text: row.modelData.title
                            font.pixelSize: row.isChapter ? 12 : 14
                            font.weight: row.isChapter ? Font.Bold : (row.active ? Font.DemiBold : Font.Normal)
                            font.letterSpacing: row.isChapter ? 1 : 0
                            color: row.isChapter ? Theme.textFaint
                                 : row.active ? Theme.accent
                                 : row.ready ? Theme.text : Theme.textFaint
                        }
                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            visible: !row.isChapter && !row.ready
                            text: qsTr("待写")
                            font.pixelSize: 11
                            color: Theme.textFaint
                        }
                        HoverHandler {
                            id: rowHover
                            enabled: !row.isChapter
                            cursorShape: row.ready ? Qt.PointingHandCursor : Qt.ArrowCursor
                        }
                        ToolTip.visible: rowHover.hovered && row.modelData.from.length > 0
                        ToolTip.delay: 600
                        ToolTip.text: qsTr("迁移自旧版：") + row.modelData.from
                        TapHandler {
                            enabled: row.ready
                            onTapped: page.open(row.modelData.file, page.volume.title, row.modelData.chapter)
                        }
                    }
                }
            }
        }

        // ---------------- 正文 ----------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: content
                anchors.fill: parent
                asynchronous: false
                onLoaded: {
                    if (item && item.volumeTitle !== undefined) {
                        item.volumeTitle = page.currentVolume
                        item.chapterTitle = page.currentChapter
                    }
                }
                onStatusChanged: {
                    if (status === Loader.Error)
                        page.loadError = qsTr("正文加载失败，请查看终端输出的 QML 错误。")
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: 10
                visible: content.status !== Loader.Ready
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: page.loadError.length > 0 ? page.loadError : qsTr("从左侧目录选择一节")
                    font.pixelSize: Theme.fontBody
                    color: page.loadError.length > 0 ? Theme.danger : Theme.textFaint
                }
            }
        }
    }
}
