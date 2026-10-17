import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick3D
import SpatialLessons
ApplicationWindow {
    id: window
    width: 1040; height: 800; minimumWidth: 820; minimumHeight: 650; visible: true
    title: "三维场景：坐标、相机与交互"
    property bool selected: false
    property real dragYaw: 0
    property real dragX: 0
    ProjectionBridge { id: bridge }
    readonly property var points: bridge.project(yaw.value, distance.value, cameraX.value, focal.value)
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 18; spacing: 12
        Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "单位约定：1 场景单位 = 1 mm。先点击选中物体，再水平拖动旋转。右侧圆点来自共享 OpenCV 投影；正交模式只观察三维场景，不显示透视投影点。" }
        ComboBox { id: lesson; model: ["场景与坐标", "父子变换", "透视与正交", "材质与光照", "拾取与拖动", "界面与投影联动"] }
        GridLayout {
            columns: 4; Layout.fillWidth: true
            Label { text: "物体绕 Y 旋转" }
            Slider { id: yaw; from: -70; to: 70; value: 20; Layout.fillWidth: true }
            Label { text: "相机距离 mm" }
            Slider { id: distance; from: 250; to: 850; value: 500; Layout.fillWidth: true }
            Label { text: "相机横移 mm" }
            Slider { id: cameraX; from: -180; to: 180; value: 0; Layout.fillWidth: true }
            Label { text: "焦距 px（640×480）" }
            Slider { id: focal; from: 250; to: 850; value: 500; Layout.fillWidth: true }
        }
        Item {
            Layout.fillWidth: true
            implicitHeight: options.implicitHeight
            Flow {
                id: options; width: parent.width; spacing: 8
                CheckBox { id: ortho; text: "正交投影" }
                CheckBox { id: orbit; text: "平移受父旋转影响"; enabled: lesson.currentIndex === 1 }
                CheckBox { id: checker; text: "棋盘纹理" }
                CheckBox { id: unlit; text: "不受光照影响" }
                Label { text: "粗糙度" }
                Slider { id: roughness; from: 0; to: 1; value: .5; width: 120 }
                Label { text: "透明度" }
                Slider { id: alpha; from: .15; to: 1; value: 1; width: 110 }
                Button { text: "重置"; onClicked: { yaw.value=20; distance.value=500; cameraX.value=0; focal.value=500; ortho.checked=false; orbit.checked=false; unlit.checked=false; checker.checked=false; roughness.value=.5; alpha.value=1; window.selected=false } }
            }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            // [region scene]
            View3D {
                id: view
                Layout.fillWidth: true; Layout.fillHeight: true
                environment: SceneEnvironment { clearColor: "#263342"; backgroundMode: SceneEnvironment.Color }
                camera: ortho.checked ? orthographic : perspective
                // [region camera]
                PerspectiveCamera {
                    id: perspective
                    position: Qt.vector3d(cameraX.value,0,distance.value)
                    fieldOfView: 2*Math.atan(240/focal.value)*180/Math.PI
                    fieldOfViewOrientation: PerspectiveCamera.Vertical
                    clipNear: 1; clipFar: 2000
                }
                // [endregion]
                OrthographicCamera {
                    id: orthographic; position: perspective.position
                    horizontalMagnification: 1; verticalMagnification: 1; clipNear: 1; clipFar: 2000
                }
                DirectionalLight { eulerRotation: Qt.vector3d(-25,-25,0); brightness: 1.2 }
                Texture {
                    id: gridTexture
                    sourceItem: Item {
                        width: 128; height: 128
                        Repeater {
                            model: 64
                            Rectangle {
                                required property int index
                                x: (index%8)*16; y: Math.floor(index/8)*16; width: 16; height: 16
                                color: (index%8+Math.floor(index/8))%2 ? "#efefef" : "#303030"
                            }
                        }
                    }
                }
                Node {
                    id: pivot
                    eulerRotation.y: yaw.value
                    // [region hierarchy]
                    Model {
                        id: plate; source: "#Cube"; pickable: true
                        scale: Qt.vector3d(2,1.5,.05)
                        x: lesson.currentIndex === 1 && orbit.checked ? 100 : 0
                        materials: PrincipledMaterial {
                            baseColor: window.selected ? "#ffb85c" : "#63c9ac"
                            baseColorMap: checker.checked ? gridTexture : null
                            roughness: roughness.value; metalness: .15; opacity: alpha.value
                            lighting: unlit.checked ? PrincipledMaterial.NoLighting : PrincipledMaterial.FragmentLighting
                        }
                    }
                    // [endregion]
                }
                // [region picking]
                MouseArea {
                    anchors.fill: parent
                    onPressed: mouse => {
                        window.selected=view.pick(mouse.x,mouse.y).objectHit === plate
                        window.dragYaw=yaw.value; window.dragX=mouse.x
                    }
                    onPositionChanged: mouse => {
                        if (pressed && window.selected)
                            yaw.value=Math.max(yaw.from,Math.min(yaw.to,window.dragYaw+(mouse.x-window.dragX)*.25))
                    }
                }
                // [endregion]
            }
            // [endregion]
            Rectangle {
                Layout.preferredWidth: 320; Layout.preferredHeight: 240
                color: "#202830"; border.color: "#78909c"
                Repeater {
                    model: !ortho.checked && !(lesson.currentIndex===1 && orbit.checked) ? window.points : []
                    Rectangle {
                        required property var modelData
                        width: 8; height: 8; radius: 4; color: "#8be0bc"
                        x: modelData.x/2-width/2; y: modelData.y/2-height/2; visible: modelData.valid
                    }
                }
            }
        }
        Label {
            Layout.fillWidth: true; wrapMode: Text.Wrap
            text: ["场景树保存对象关系；View3D 把三维场景交给二维布局。", "勾选后，局部 x=100 的平移会跟随父节点旋转，描述的是绕父原点运动。", "透视近大远小；正交改变距离不改变物体的投影尺度。", "材质参数改变表面外观；透明度不等于生成一个具有物理正确折射的玻璃。", "pick 返回场景命中的对象；这里把水平拖动约束为绕 Y 旋转，不声称像素位移就是世界距离。", "两侧共用距离、横移、旋转和焦距；左边视口宽高比可变，右边固定 640×480 数值投影，两边不可直接按窗口像素比较。"][lesson.currentIndex]
        }
    }
}
