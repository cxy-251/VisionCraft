import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Lesson 1.0
ApplicationWindow {
    id: window
    visible: true; width: 760; height: 720; minimumHeight: 650
    title: "QML：数组、ListModel与C++模型"
    property var samples: [{label:"A",amount:1}]
    property int observed: 1
    LessonModel { id: numbers }
    ListModel { id: local; ListElement { label: "内置模型"; amount: 1 } }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; spacing: 10
        Label { text: "数据改变之后，谁通知界面？"; font.pixelSize: 22 }
        // [region array]
        Label { text: "JS数组绑定显示：" + window.samples[0].amount + "；按钮直接读到：" + window.observed }
        RowLayout {
            Button { text: "原地修改"; onClicked: { window.samples[0].amount++;window.observed=window.samples[0].amount } }
            Button { text: "替换整个数组"; onClicked: { window.samples=window.samples.map(n=>({label:n.label,amount:n.amount+1}));window.observed=window.samples[0].amount } }
        }
        // [endregion]
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "普通JS对象字段没有QML变化信号。数组原地修改后，绑定可能仍显示旧值；重新赋值samples才发出属性变化。" }
        // [region listmodel]
        RowLayout {
            Button { text: "ListModel.setProperty"; onClicked: local.setProperty(0,"amount",local.get(0).amount+1) }
            Repeater {
                model: local
                delegate: Label { required property string label; required property int amount; text: label + "：" + amount }
            }
        }
        // [endregion]
        Label { text: "下面由C++模型持有数据：改数值、加行、删行"; font.pixelSize: 18 }
        RowLayout {
            Button { text: "加一行（最多20行）"; onClicked: numbers.appendRow() }
            Button { text: "删最后一行"; onClicked: numbers.removeLast() }
        }
        // [region delegate]
        ListView {
            id: list; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 6
            model: numbers
            ScrollBar.vertical: ScrollBar {}
            delegate: RowLayout {
                id: row
                required property int index
                required property string label
                required property int amount
                width: list.width
                Label { text: row.label; Layout.fillWidth: true }
                SpinBox { from: 0; to: 100; value: row.amount; onValueModified: numbers.setAmount(row.index,value) }
            }
        }
        // [endregion]
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "Repeater为每项创建对象；ListView按视口需求创建委托，可能预取或缓存。二者都不应把长期数据放在委托内部。" }
    }
}
