import QtQuick
import QtQuick.Layouts
import VisionCraft

// 一排配图：files 里的每个文件一张图，等宽并排，图下各有一行说明。
//   Figure { files: ["handbook/opencv/figures/a.png", "…/b.png"]; captions: ["原图", "二值化"] }
// 图片是项目里真实生成的文件（例如 examples/opencv/inspect_steps 导出的），和代码一样按路径加载。
ColumnLayout {
    id: fig
    property var files: []
    property var captions: []
    property string caption            // 整组图下面的总说明（可选）
    property int columns: Math.min(files.length, 3)

    Layout.fillWidth: true
    spacing: 6

    GridLayout {
        Layout.fillWidth: true
        columns: fig.columns
        columnSpacing: 10
        rowSpacing: 10

        Repeater {
            model: fig.files
            delegate: ColumnLayout {
                required property string modelData
                required property int index
                Layout.fillWidth: true
                Layout.preferredWidth: 1      // 各列等宽
                spacing: 4

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: img.implicitWidth > 0 ? width * img.implicitHeight / img.implicitWidth : 120
                    color: Theme.codeBg
                    radius: 6
                    border.color: Theme.border
                    clip: true
                    Image {
                        id: img
                        anchors.fill: parent
                        anchors.margins: 1
                        source: SourceProvider.contentUrl(modelData)
                        fillMode: Image.PreserveAspectFit
                        smooth: false            // 放大时不插值，像素看得清
                        asynchronous: true
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: img.status === Image.Error
                        text: qsTr("（找不到图片：%1）").arg(modelData)
                        color: Theme.danger
                        font.pixelSize: Theme.fontSmall
                    }
                }
                Text {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: index < fig.captions.length ? fig.captions[index] : ""
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSmall
                    color: Theme.textMuted
                }
            }
        }
    }

    Text {
        Layout.fillWidth: true
        visible: text.length > 0
        text: fig.caption
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSmall
        color: Theme.textMuted
    }
}
