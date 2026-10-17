import QtQuick
import VisionCraft
Section {
    title: qsTr("文本输入：编辑状态、校验与明确提交")
    lead: qsTr("用户为了把120改成80，可能先删成空字符串。输入过程不是业务数值的连续有效版本；把草稿和已提交值分开，才能允许编辑而不误触发计算。")
    Why { text: qsTr("已有绑定与状态课程解释属性变化。本节补输入的三种有效性状态与提交边界，不把每次textChanged直接当作新的算法参数。使用example_qml_forms第二页，范围为10–240。") }
    CodeRef { file: "examples/qml/forms/ValidationPage.qml"; region: "draft" }
    KeyPoints {
        label: qsTr("编辑不是只有对或错")
        points: [
            qsTr("校验器区分Invalid、Intermediate和Acceptable。空字符串、尚未输入完整的数字可能处于中间态；不能把acceptableInput为false等同于用户已经犯错。"),
            qsTr("TextInput类控件允许校验器认可的中间编辑状态，但阻止被判断为Invalid的交互编辑。脚本赋值和外部数据仍需在提交边界检查，不能把编辑约束当作所有输入的防线。"),
            qsTr("acceptableInput表示当前文本达到控件的可接受条件，不等于业务操作已经执行。示例按Enter或点提交才更新committed；清空或继续打字不会覆盖它。"),
            qsTr("带校验器时accepted和editingFinished的发出受输入可接受性影响；editingFinished还可能来自焦点变化，不能把它无条件等同于用户点击保存。本例仅用accepted与按钮明确提交。")
        ]
    }
    CodeRef { file: "examples/qml/forms/ValidationPage.qml"; region: "commit" }
    Para { text: qsTr("提交先检查acceptableInput，再检查转换后的整数和范围。locale设为C以固定教学环境；Number转换失败仍会被拒绝，不能假定校验器认可的本地化写法都能被JavaScript数值解析接受。完整的小数或分组符输入，应让校验与QLocale解析遵守同一地区规则。本例只演示整数。") }
    Pitfall { text: qsTr("不要在onTextChanged里不断把草稿替换成格式化数值：会干扰光标、清空操作和输入法组合。也不要同时绑定text到committed，又在每次编辑时回写committed，形成双向更新和意外重算。示例采用明确的提交与撤销。") }
    Try { task: qsTr("committed为120时清空输入框，后台是否应立刻收到0？为什么撤销草稿可以恢复120？"); answerNote: qsTr("不应。空草稿是编辑状态，没有发生提交。已提交值独立保存，撤销只把文本恢复为它的表示，不需要伪造一次新业务提交。") }
    Para { onLinkActivated: link => Qt.openUrlExternally(link); text: qsTr("信号与校验行为参考：<a href='https://doc.qt.io/qt-6/qml-qtquick-textinput.html'>Qt TextInput</a>。练习中的可输入中间形式由校验器判定，不要求所有键盘与输入法有相同事件序列。") }
    InSystem { text: qsTr("examples/qml/forms/ValidationPage.qml。不将这套草稿示例直接替换工位滑条；滑条连续预览与表单明确提交服务不同操作。") }
}
