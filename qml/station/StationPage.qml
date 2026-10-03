import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft

// 工位：左边是传送带（被检测的画面），右边是检测结果和统计
Item {
    id: page

    readonly property var defectNames: ["合格", "划痕", "缺口", "污点", "偏心", "未找到"]

    // [region grab]
    // 检测请求：截取传送带画面交给 C++。截的是界面上真正显示的东西
    Connections {
        target: Station
        function onGrabRequested() {
            Qt.callLater(function() {
                conveyor.grabToImage(function(result) { Station.inspectGrab(result.image) }, Qt.size(480, 360))
            })
        }
    }
    // [endregion]
    // 板子上按 KEY0 = 检测一次
    Connections {
        target: DeviceLink
        function onEventReceived(e) {
            if (e.name === "key" && e.key === 0 && e.down)
                Station.inspectNow()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 24

        // ================= 左：传送带 =================
        ColumnLayout {
            Layout.fillHeight: true
            // 嵌套布局里只要有子项 fillWidth，布局自己默认也会 fillWidth，所以这里必须显式关掉
            Layout.fillWidth: false
            Layout.preferredWidth: (page.width - 64 - 24) * 0.5
            spacing: 14

            RowLayout {
                Text { text: qsTr("工位"); font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold; color: Theme.text }
                Item { Layout.fillWidth: true }
                Rectangle { width: 10; height: 10; radius: 5
                    color: DeviceLink.state === DeviceLink.Connected ? Theme.tryIt : Theme.textFaint }
                Text {
                    text: DeviceLink.state === DeviceLink.Connected ? qsTr("板子已连接：KEY0 触发检测，结果显示在板子屏幕上")
                                                                  : qsTr("板子未连接（在「设备」页连接）")
                    font.pixelSize: 12; color: Theme.textMuted
                }
            }

            Text { text: qsTr("传送带（检测的是这块画面的截图）"); font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: width * 0.75
                color: "black"
                radius: 6
                ImageView {
                    id: conveyor
                    anchors.fill: parent
                    image: Station.partImage
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 10
                Btn { text: qsTr("检测一次"); primary: true; enabled: !Station.busy; onClicked: Station.inspectNow() }
                Btn { text: Station.running ? qsTr("停止产线") : qsTr("启动产线"); onClicked: Station.running = !Station.running }
                Btn { text: qsTr("换一件"); enabled: !Station.busy; onClicked: Station.nextPart() }
                Btn { text: qsTr("清零统计"); onClicked: Station.resetStats() }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 12
                rowSpacing: 4
                SettingLabel { text: qsTr("缺陷率") }
                Slider { Layout.fillWidth: true; from: 0; to: 1; value: Station.defectRate; onMoved: Station.defectRate = value }
                SettingValue { text: Math.round(Station.defectRate * 100) + "%" }
                SettingLabel { text: qsTr("缺陷难度") }
                Slider { Layout.fillWidth: true; from: 0; to: 1; value: Station.difficulty; onMoved: Station.difficulty = value }
                SettingValue { text: Station.difficulty < 0.34 ? qsTr("明显") : Station.difficulty < 0.67 ? qsTr("中等") : qsTr("接近阈值") }
                SettingLabel { text: qsTr("产线节拍") }
                Slider { Layout.fillWidth: true; from: 200; to: 3000; stepSize: 100; value: Station.intervalMs; onMoved: Station.intervalMs = value }
                SettingValue { text: Station.intervalMs + " ms" }
            }

            // 检测配方：阈值存在板子的 EEPROM 里
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: recipeCol.implicitHeight + 28
                radius: Theme.radius
                color: Theme.surface
                border.color: Theme.border
                ColumnLayout {
                    id: recipeCol
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 6
                    RowLayout {
                        Text { text: qsTr("检测配方"); font.pixelSize: Theme.fontSmall; font.weight: Font.Bold; color: Theme.textMuted }
                        Text { text: qsTr("来源：") + Station.recipeSource; font.pixelSize: 12
                               color: Station.recipeSource === qsTr("板子") ? Theme.tryIt : Theme.workerLane }
                        Item { Layout.fillWidth: true }
                        Btn { text: qsTr("从板子读取"); enabled: DeviceLink.state === DeviceLink.Connected; onClicked: Station.loadRecipeFromBoard() }
                        Btn { text: qsTr("写入板子"); enabled: DeviceLink.state === DeviceLink.Connected; onClicked: Station.saveRecipeToBoard() }
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 12
                        rowSpacing: 0
                        SettingLabel { text: qsTr("表面阈值") }
                        Slider { Layout.fillWidth: true; from: 5; to: 60; stepSize: 1; value: Station.recipe.surfaceThreshold
                                 onMoved: Station.setRecipeValue("surfaceThreshold", value) }
                        SettingValue { text: Station.recipe.surfaceThreshold }
                        SettingLabel { text: qsTr("偏心阈值") }
                        Slider { Layout.fillWidth: true; from: 2; to: 15; stepSize: 0.5; value: Station.recipe.maxCenterOffset
                                 onMoved: Station.setRecipeValue("maxCenterOffset", value) }
                        SettingValue { text: Station.recipe.maxCenterOffset.toFixed(1) + " px" }
                        SettingLabel { text: qsTr("缺口深度") }
                        Slider { Layout.fillWidth: true; from: 2; to: 15; stepSize: 0.5; value: Station.recipe.minChipDepth
                                 onMoved: Station.setRecipeValue("minChipDepth", value) }
                        SettingValue { text: Station.recipe.minChipDepth.toFixed(1) + " px" }
                    }
                }
            }
            Item { Layout.fillHeight: true }
        }

        // ================= 右：结果 =================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            // 判定横幅
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 86
                radius: Theme.radius
                readonly property bool has: Station.lastResult.ok !== undefined
                color: !has ? Theme.surface : Station.lastResult.ok ? Theme.tryItBg : Theme.pitfallBg
                border.color: Theme.border
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 18
                    Text {
                        text: !parent.parent.has ? "—" : Station.lastResult.ok ? "OK" : "NG"
                        font.pixelSize: 44; font.weight: Font.Black
                        color: !parent.parent.has ? Theme.textFaint : Station.lastResult.ok ? Theme.tryIt : Theme.danger
                    }
                    ColumnLayout {
                        spacing: 4
                        Text {
                            text: parent.parent.parent.has ? qsTr("判定：%1　　标准答案：%2").arg(Station.lastResult.defectName).arg(Station.lastResult.truthName)
                                                          : qsTr("还没有检测")
                            font.pixelSize: Theme.fontBody; color: Theme.text
                        }
                        Text {
                            visible: parent.parent.parent.has
                            text: (Station.lastResult.correct ? qsTr("判对了") : qsTr("判错了")) + qsTr("　·　检测耗时 %1 ms").arg((Station.lastResult.ms || 0).toFixed(2))
                            font.pixelSize: Theme.fontSmall
                            color: Station.lastResult.correct ? Theme.textMuted : Theme.danger
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: width * 0.75
                Layout.maximumHeight: page.height * 0.45
                color: "black"
                radius: 6
                ImageView { anchors.fill: parent; image: Station.resultImage }
            }

            // 统计
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Stat { title: qsTr("已检测"); value: Station.stats.total }
                Stat { title: qsTr("判为不合格"); value: Station.stats.ng }
                Stat { title: qsTr("判对率"); value: Station.stats.total ? (Station.stats.accuracy * 100).toFixed(1) + "%" : "—" }
            }

            // 混淆矩阵：行 = 标准答案，列 = 判定
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: matrix.implicitHeight + 28
                radius: Theme.radius
                color: Theme.surface
                border.color: Theme.border
                GridLayout {
                    id: matrix
                    anchors.fill: parent
                    anchors.margins: 14
                    columns: 7
                    columnSpacing: 0
                    rowSpacing: 2
                    Text { text: qsTr("答案＼判定"); font.pixelSize: 11; color: Theme.textFaint; Layout.preferredWidth: 80 }
                    Repeater {
                        model: page.defectNames
                        delegate: Text { required property string modelData
                            text: modelData; font.pixelSize: 11; color: Theme.textMuted
                            horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                    }
                    Repeater {
                        model: 5
                        delegate: Repeater {
                            id: rowRep
                            required property int index
                            readonly property int truth: index
                            model: 7
                            delegate: Text {
                                required property int index
                                readonly property int count: index === 0 ? 0 : (Station.stats.confusion ? Station.stats.confusion[rowRep.truth][index - 1] : 0)
                                text: index === 0 ? page.defectNames[rowRep.truth] : count
                                Layout.fillWidth: index > 0
                                Layout.preferredWidth: index === 0 ? 80 : -1
                                horizontalAlignment: index === 0 ? Text.AlignLeft : Text.AlignHCenter
                                font.pixelSize: 12
                                font.family: index === 0 ? "" : SourceProvider.monoFont
                                font.weight: index - 1 === rowRep.truth && count > 0 ? Font.Bold : Font.Normal
                                color: index === 0 ? Theme.textMuted
                                     : count === 0 ? Theme.textFaint
                                     : index - 1 === rowRep.truth ? Theme.tryIt : Theme.danger
                            }
                        }
                    }
                }
            }
            Item { Layout.fillHeight: true }
        }
    }

    component Btn: Rectangle {
        id: b
        property string text
        property bool primary: false
        signal clicked
        opacity: enabled ? 1 : 0.45
        implicitWidth: t.implicitWidth + 30
        implicitHeight: 38
        radius: 8
        color: primary ? (h.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent) : (h.hovered ? Theme.surfaceAlt : Theme.surface)
        border.color: primary ? Theme.accent : Theme.border
        Text { id: t; anchors.centerIn: parent; text: b.text; font.pixelSize: Theme.fontSmall; font.weight: Font.DemiBold
               color: b.primary ? (Theme.dark ? "#0f172a" : "#ffffff") : Theme.text }
        HoverHandler { id: h; cursorShape: b.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor }
        TapHandler { enabled: b.enabled; onTapped: b.clicked() }
    }
    component SettingLabel: Text { font.pixelSize: Theme.fontSmall; color: Theme.textMuted }
    component SettingValue: Text { font.pixelSize: Theme.fontSmall; color: Theme.text; Layout.preferredWidth: 70 }
    component Stat: Rectangle {
        property string title
        property var value
        Layout.fillWidth: true
        implicitHeight: 72
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
        Column {
            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter; anchors.leftMargin: 16
            spacing: 2
            Text { text: parent.parent.title; font.pixelSize: 12; color: Theme.textMuted }
            Text { text: parent.parent.value === undefined ? "—" : parent.parent.value; font.pixelSize: 24; font.weight: Font.DemiBold; color: Theme.text }
        }
    }
}
