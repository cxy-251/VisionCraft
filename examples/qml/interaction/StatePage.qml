import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: page
    property int level: 40
    property SystemPalette colors: SystemPalette {}
    ColumnLayout {
        anchors.fill: parent; spacing: 16
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "默认宽度绑定level×3；强调态覆盖宽度，离开后恢复进入前的绑定。" }
        RowLayout {
            Label { text: "level" }
            SpinBox { from: 20; to: 180; value: page.level; onValueModified: page.level=value }
            CheckBox { id: emphasize; text: "强调态" }
        }
        // [region state]
        Rectangle {
            id: field; Layout.fillWidth: true; Layout.preferredHeight: 100; color: page.colors.alternateBase
            Rectangle {
                id: bar; x: 12; y: 24; height: 48
                width: page.level*3; color: page.colors.mid
                state: emphasize.checked ? "emphasized" : ""
                states: State {
                    name: "emphasized"
                    PropertyChanges {
                        target: bar; width: field.width-24; color: page.colors.highlight
                        restoreEntryValues: true
                    }
                }
            }
        }
        // [endregion]
        Label { text: "当前state="+(bar.state || "默认")+"；宽度="+bar.width.toFixed(1)+"；level×3="+(page.level*3) }
        // [region assignment]
        RowLayout {
            Button { text: "赋值width=90"; enabled: !emphasize.checked; onClicked: bar.width=90 }
            Button { text: "恢复width绑定"; enabled: !emphasize.checked; onClicked: bar.width=Qt.binding(()=>page.level*3) }
        }
        // [endregion]
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "先赋值90再改level：绑定已被赋值替换。此后进入、离开强调态，恢复的是进入前的90，不是凭空重新生成最初绑定。点恢复绑定后再比较。" }
        Item { Layout.fillHeight: true }
    }
}
