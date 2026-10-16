import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VisionCraft
ToolPage {
    title: qsTr("开发工具箱")
    description: qsTr("JSON校验、四空格格式化与单行压缩。解析失败保留上次有效输出，详细错误显示在状态栏。")
    DevTools { id: tools }
    RowLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 18
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            ToolNote { text: qsTr("输入JSON") }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true
                TextArea { id: input; text: '{"project":"VisionCraft","enabled":true,"features":["Qt Quick","OpenCV"]}'; color: Theme.text; selectByMouse: true; font.family: "monospace"; wrapMode: TextEdit.Wrap; background: Rectangle { color: Theme.surface; border.color: Theme.border; radius: 6 } }
            }
            RowLayout {
                ToolAction { text: qsTr("美化缩进"); onClicked: tools.formatJson(input.text,false) }
                ToolAction { text: qsTr("单行压缩"); onClicked: tools.formatJson(input.text,true) }
                ToolAction { text: qsTr("复制输出"); enabled: output.length > 0; onClicked: { output.selectAll();output.copy();output.deselect() } }
            }
            ToolNote { text: tools.status || qsTr("等待解析") }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true
                TextArea { id: output; text: tools.output; readOnly: true; color: Theme.text; selectByMouse: true; font.family: "monospace"; wrapMode: TextEdit.Wrap; background: Rectangle { color: Theme.codeBg; border.color: Theme.border; radius: 6 } }
            }
        }
        ColumnLayout {
            Layout.preferredWidth: 270; Layout.fillHeight: true
            ToolNote { text: qsTr("运行环境"); font.pixelSize: Theme.fontHeading }
            ToolNote { id: info; text: tools.systemInfo }
            ToolAction { text: qsTr("刷新系统信息"); onClicked: tools.refreshSystemInfo() }
            Item { Layout.fillHeight: true }
        }
    }
}
