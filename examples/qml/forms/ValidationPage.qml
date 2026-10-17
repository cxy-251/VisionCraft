import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: page
    property int committed: 120
    property string status: "草稿变化不会直接修改已提交值"
    spacing: 14
    // [region commit]
    function commit() {
        if (!draft.acceptableInput) { status = "拒绝提交：需要10–240的整数"; return }
        const value = Number(draft.text)
        if (!Number.isInteger(value) || value < 10 || value > 240) {
            status = "数值检查未通过"; return
        }
        committed = value
        status = "已明确提交：" + value
    }
    // [endregion]
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "数值范围10–240。尝试清空、输入1、再输入120。中间编辑状态可以存在，但只有acceptableInput为true时才允许提交。" }
    // [region draft]
    TextField {
        id: draft
        Layout.fillWidth: true
        text: "120"
        validator: IntValidator { bottom: 10; top: 240; locale: "C" }
        Accessible.name: "阈值草稿"
        onAccepted: page.commit()
    }
    Label { text: "草稿：\"" + draft.text + "\"；acceptableInput=" + draft.acceptableInput }
    RowLayout {
        Button { text: "提交"; enabled: draft.acceptableInput; onClicked: page.commit() }
        Button { text: "撤销草稿"; onClicked: { draft.text = String(page.committed); page.status = "已恢复最近提交值" } }
    }
    Label { text: "已提交值：" + page.committed }
    // [endregion]
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: page.status }
    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "输入法组合中的文本不等于最终业务值。此例只接受整数；小数的区域格式应另行约定，不能直接换一个小数校验器后仍盲用Number解析。" }
    Item { Layout.fillHeight: true }
}
