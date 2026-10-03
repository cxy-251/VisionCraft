// 不需要窗口：QtObject 是最简单的 QML 对象，只有属性
import QtQml

QtObject {
    id: root

    // [region binding]
    property int parentWidth: 400
    property int childWidth: parentWidth / 2          // 绑定：parentWidth 变，它跟着变
    property string label: "宽度 " + childWidth        // 绑定可以连锁
    // [endregion]

    // [region handler]
    // 每个属性都自带一个「变化」信号，on<属性名>Changed 是它的处理函数
    onChildWidthChanged: console.log("  onChildWidthChanged：childWidth =", childWidth)
    // [endregion]

    // [region readonly]
    readonly property int doubled: childWidth * 2     // readonly：只能由绑定决定，不能从外面赋值
    function tryWrite() {
        doubled = 1
    }
    // [endregion]

    // [region break]
    function setFixed() {
        childWidth = 100                               // 赋值：从此 childWidth 只是一个普通的值，绑定没了
    }
    function restore() {
        childWidth = Qt.binding(() => parentWidth / 2) // 用 Qt.binding 重新建立绑定
    }
    // [endregion]
}
