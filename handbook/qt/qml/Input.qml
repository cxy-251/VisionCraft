import QtQuick
import VisionCraft
Section {
    title: qsTr("输入事件：点击、拖动与事件归属")
    lead: qsTr("画面上有父子关系，不等于所有事件都会像函数调用一样逐级冒泡。先区分原始按下、组合点击与手势识别，再观察谁接受了这一次输入。")
    Why { text: qsTr("Widgets事件章已有事件派发与过滤。本节补Qt Quick的输入模型：上层MouseArea挡住下层、拖动影响轻点、滚轮被图像查看器接走，都应从接受与抓取关系解释，不能只按视觉层级猜。") }
    CodeRef { file: "examples/qml/interaction/InputPage.qml"; region: "mouse" }
    KeyPoints {
        label: qsTr("两种传递不能混淆")
        points: [
            qsTr("pressed是原始按下处理。上层在onPressed里设置mouse.accepted=false，表示它拒绝这次按下，其他合适的接收者才有机会处理；上层通常也不会因此形成自己的完整点击序列。"),
            qsTr("clicked是按下与释放组合出的点击。propagateComposedEvents配合onClicked里的accepted=false，可以把这种组合事件继续提供给下方MouseArea；它不会把每一次原始移动也复制下去。"),
            qsTr("示例把两种开关分开：先都关闭点击上层，再只开clicked传递，最后观察拒绝pressed。日志表达接受过程，不要求所有平台产生完全相同数量的移动事件。"),
            qsTr("视觉父子关系、QObject所有权与输入接收是不同关系。一个对象属于父项，并不意味着父项必然收到同一次clicked。")
        ]
    }
    CodeRef { file: "examples/qml/interaction/InputPage.qml"; region: "drag" }
    Para { text: qsTr("第二个区域在同一方块上使用TapHandler与DragHandler。轻点与超过拖动阈值后形成的拖动不同；Handler负责识别与抓取输入，target负责被移动的对象，xAxis范围限制移动几何。只有拖动识别器激活时才算开始拖动，不能用任意position变化当作拖动成立。") }
    KeyPoints {
        label: qsTr("与滚动和坐标一起理解")
        points: [
            qsTr("MouseArea的mouse.x/y是该区域的局部坐标；拖动目标可以在另一个坐标空间。跨项换算应使用mapToItem/mapFromItem等映射，不能随手把局部坐标当窗口坐标。"),
            qsTr("滚轮的angleDelta与像素滚动量是不同信息，触控板不一定表现为每次固定120。示例只记录角度量，不把它当作所有设备统一的像素距离。"),
            qsTr("在Flickable里拖动图像时，滚动容器与图像操作可能竞争抓取。应先决定普通滚轮是滚页还是缩放、什么手势启动平移，再配置处理；不是所有区域都无条件接受事件。")
        ]
    }
    Pitfall { text: qsTr("不要为了看到下层点击就把所有pressed都拒绝：上层因此可能失去本来需要的释放、拖动或点击逻辑。也不要把propagateComposedEvents误认为通用的事件冒泡开关。") }
    Try { task: qsTr("示例中只启用clicked传递时，下层能收到clicked，是否意味着它也收到同样的pressed序列？拖动方块后为什么不应再把释放当成一次业务点击？"); answerNote: qsTr("组合点击的传递与原始序列不同，不能据此推断下层经历了相同按下过程。拖动识别已表达另一种用户意图，应按识别器的完成/取消规则处理，避免同时触发拖动和点击业务动作。") }
    InSystem { text: qsTr("运行build/examples/qml/interaction/example_qml_interaction的输入事件页。对照ImageView的wheelEvent、mousePressEvent与mapToImage，以及ImageViewer外围的页面滚动，理解已有交互约定。") }
}
