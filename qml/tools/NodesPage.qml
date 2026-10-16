import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import VisionCraft
ToolPage {
    id: page
    title: qsTr("节点管线")
    description: qsTr("拖动节点布局，用起点/终点连接算子，再运行整张图。每个算子接收一个输入，支持分支；循环、断开输入与非法连线会明确报错。屏幕源只在点击运行后捕获。")
    minimumContentHeight: 1050
    onVisibleChanged: if (!visible) runner.stop()
    property var nodes: [
        {id:1,type:"sample",label:"1 · 工件源",x:30,y:35,params:{hasDefect:true}},
        {id:2,type:"gray",label:"2 · 灰度",x:260,y:35,params:{}},
        {id:3,type:"threshold",label:"3 · 二值化",x:490,y:35,params:{otsu:true}},
        {id:4,type:"contours",label:"4 · 轮廓",x:720,y:35,params:{minArea:80,maxArea:80000}}]
    property var edges: [{from:1,to:2},{from:2,to:3},{from:3,to:4}]
    property int selected: 4
    property int nextId: 5
    property string editStatus: ""
    readonly property var selectedNode: nodes.find(n => n.id === selected) || {type:"",params:{}}
    readonly property var types: ["sample","screen","gray","blur","canny","threshold","morph","contours","qr"]
    readonly property var names: [qsTr("工件源"),qsTr("屏幕源"),qsTr("灰度"),qsTr("高斯"),"Canny",qsTr("二值化"),qsTr("形态学"),qsTr("轮廓"),qsTr("二维码")]
    function preset() {
        runner.stop()
        nodes=[{id:1,type:"sample",label:"1 · 工件源",x:30,y:40,params:{hasDefect:true}},
            {id:2,type:"gray",label:"2 · 灰度",x:260,y:40,params:{}},
            {id:3,type:"blur",label:"3 · 高斯",x:490,y:40,params:{ksize:7}},
            {id:4,type:"canny",label:"4 · Canny",x:720,y:40,params:{low:50,high:140}},
            {id:5,type:"contours",label:"5 · 轮廓",x:950,y:40,params:{minArea:80,maxArea:80000}}]
        edges=[{from:1,to:2},{from:2,to:3},{from:3,to:4},{from:4,to:5}];selected=5;nextId=6;wires.requestPaint()
    }
    function clear() { runner.stop(); nodes=[]; edges=[]; selected=-1; wires.requestPaint() }
    function addNode() {
        if(nodes.length >= 24) { editStatus = qsTr("最多24个节点"); return }
        const id = nextId++
        nodes = nodes.concat([{id:id,type:types[typeBox.currentIndex],label:id+" · "+names[typeBox.currentIndex],x:30+(nodes.length%5)*230,y:150+Math.floor(nodes.length/5)*120,params:{}}])
        selected=id; wires.requestPaint()
    }
    function removeNode() {
        nodes = nodes.filter(n => n.id !== selected)
        edges = edges.filter(e => e.from !== selected && e.to !== selected)
        selected = nodes.length ? nodes[0].id : -1; wires.requestPaint()
    }
    function connectNodes() {
        const from = fromBox.currentValue, to = toBox.currentValue
        if(from === undefined || to === undefined || from === to) { editStatus=qsTr("请选择不同的起点和终点");return }
        const target=nodes.find(n=>n.id===to)
        if(target.type === "sample" || target.type === "screen") { editStatus=qsTr("源节点不接收输入");return }
        edges=edges.filter(e=>e.to!==to).concat([{from:from,to:to}]);editStatus=qsTr("已连接；重新连接会替换终点原输入");wires.requestPaint()
    }
    function setParam(key, value) {
        nodes=nodes.map(n=>n.id===selected ? Object.assign({},n,{params:Object.assign({},n.params,{[key]:value})}) : n)
    }
    VisionRunner {
        id: runner; mode: "graph"
        options: ({nodes:page.nodes,edges:page.edges,output:page.selected,capture:"input"})
    }
    Flow {
        Layout.fillWidth: true; spacing: 8
        ComboBox { id: typeBox; model: page.names; palette.button: Theme.surface; palette.buttonText: Theme.text }
        ToolAction { text: qsTr("添加节点"); onClicked: page.addNode() }
        ToolAction { text: qsTr("质检预设"); onClicked: page.preset() }
        ToolAction { text: qsTr("清空画布"); onClicked: page.clear() }
        ToolAction { text: runner.running ? qsTr("停止连续运行") : qsTr("连续运行·30fps"); enabled: page.nodes.length > 0; onClicked: runner.running ? runner.stop() : runner.start(30) }
        ToolAction { text: qsTr("删除选中"); enabled: page.selected >= 0; onClicked: page.removeNode() }
        ToolAction { text: runner.busy ? qsTr("运行中…") : qsTr("运行管线"); enabled: !runner.busy && !runner.running && page.nodes.length > 0; onClicked: runner.run() }
        ToolAction { text: qsTr("导出选中输出…"); enabled: runner.resultCurrent && !runner.busy && !runner.running; onClicked: save.open() }
    }
    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("起点"); color: Theme.text }
        ComboBox { id: fromBox; model: page.nodes; textRole: "label"; valueRole: "id"; Layout.fillWidth: true; palette.button: Theme.surface; palette.buttonText: Theme.text }
        Label { text: "→"; color: Theme.text }
        ComboBox { id: toBox; model: page.nodes; textRole: "label"; valueRole: "id"; Layout.fillWidth: true; palette.button: Theme.surface; palette.buttonText: Theme.text }
        ToolAction { text: qsTr("连接"); onClicked: page.connectNodes() }
        ToolAction { text: qsTr("断开终点输入"); onClicked: { page.edges = page.edges.filter(e=>e.to!==toBox.currentValue);wires.requestPaint() } }
    }
    ToolNote { text: page.editStatus + " · " + runner.status + (runner.resultCurrent ? "" : qsTr(" · 当前管线尚未运行")) }
    Rectangle {
        Layout.fillWidth: true; Layout.preferredHeight: 300; color: Theme.codeBg; border.color: Theme.border; radius: Theme.radius; clip: true
        Flickable {
            id: canvas; anchors.fill: parent; anchors.margins: 1; contentWidth: 1500; contentHeight: 900; boundsBehavior: Flickable.StopAtBounds
            ScrollBar.horizontal: ScrollBar {}
            ScrollBar.vertical: ScrollBar {}
            Canvas {
                id: wires; width: canvas.contentWidth; height: canvas.contentHeight
                onPaint: {
                    const ctx = getContext("2d");ctx.clearRect(0,0,width,height);ctx.strokeStyle=Theme.accent;ctx.lineWidth=2
                    page.edges.forEach(e=>{
                        const a=page.nodes.find(n=>n.id===e.from),b=page.nodes.find(n=>n.id===e.to)
                        if(!a||!b)return
                        ctx.beginPath();ctx.moveTo(a.x+190,a.y+40);ctx.bezierCurveTo(a.x+240,a.y+40,b.x-50,b.y+40,b.x,b.y+40);ctx.stroke()
                    })
                }
                Connections { target: Theme; function onDarkChanged() { wires.requestPaint() } }
            }
            Repeater {
                model: page.nodes
                delegate: Rectangle {
                    id: node; required property var modelData
                    x: modelData.x; y: modelData.y; width: 190; height: 80; radius: 8
                    color: modelData.id === page.selected ? Theme.accentBg : Theme.surface; border.color: modelData.id === page.selected ? Theme.accent : Theme.border; border.width: 2
                    Column { anchors.centerIn: parent; spacing: 7
                        Text { text: node.modelData.label; color: Theme.text; font.bold: true }
                        Text { text: qsTr("● 输入             输出 ●"); color: Theme.textMuted; font.pixelSize: 11 }
                    }
                    MouseArea {
                        anchors.fill: parent; drag.target: node; drag.minimumX: 0; drag.maximumX: canvas.contentWidth-node.width; drag.minimumY: 0; drag.maximumY: canvas.contentHeight-node.height
                        onPressed: page.selected=node.modelData.id
                        onReleased: { page.nodes=page.nodes.map(n=>n.id===node.modelData.id?Object.assign({},n,{x:Math.round(node.x),y:Math.round(node.y)}):n);wires.requestPaint() }
                    }
                }
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true; Layout.fillHeight: true
        ColumnLayout {
            Layout.preferredWidth: 310; Layout.fillHeight: true
            NodeSettings { Layout.fillWidth: true; type: page.selectedNode.type; params: page.selectedNode.params; onEdited: (key,value) => page.setParam(key,value) }
            ToolNote { text: qsTr("选中 #%1 · %2").arg(page.selected).arg(page.selectedNode.type) }
            ToolNote { text: runner.resultCurrent ? JSON.stringify(runner.summary[String(page.selected)] || {}) : qsTr("参数或输出节点改变后，请重新运行。") }
            Item { Layout.fillHeight: true }
        }
        ImageViewer { Layout.fillWidth: true; Layout.fillHeight: true; image: { const version=runner.generation; return runner.nodeImage(page.selected) } }
    }
    FileDialog { id: save; fileMode: FileDialog.SaveFile; nameFilters: ["PNG (*.png)"]; defaultSuffix: "png"; onAccepted: runner.save(selectedFile) }
}
