import QtQuick
import VisionCraft
Section {
    title: qsTr("测量选择：像素、小数位与插值误差")
    lead: qsTr("能输出两位小数，不等于误差小于0.01。测量要说明被测量、坐标单位、成像与处理条件，以及用什么已知量判断偏差。")
    Why { text: qsTr("卡尺、几何变换与标定已有实例。本节把它们放到一条测量链上：像素采样→边缘定位→几何量→单位换算，不重复实现量圆工具。") }
    CodeRef { file: "examples/opencv/measure/main.cpp"; region: "caliper" }
    CodeRef { file: "examples/opencv/measure/output.txt"; from: "真实环宽"; to: "61.00"; caption: qsTr("仓库已有卡尺输出，单位为像素") }
    KeyPoints {
        label: qsTr("先把量的定义说清楚")
        points: [
            qsTr("示例真实环宽从60到61，每次增0.25；整数定位得到61、61、61、61、62，亚像素得到60.10、60.35、60.61、60.87、61.11。后者能跟随细小变化，仍有约+0.1的偏差。"),
            qsTr("这里环宽是同侧内外边缘间距：理想同心圆为R−r，不是外径减内径；径差为2(R−r)。源码R=110指外半径，不能读作外径110。"),
            qsTr("采样间距、数值显示位数、重复测量的散布和相对真值的偏差不是一个量。只有小数结果，没有已知尺寸与条件说明，不能声称达到某种准确度。"),
            qsTr("像素到毫米需要空间尺度或相机几何约束。透视、镜头畸变、物体高度变化都可能让同一个固定毫米/像素比例失效；界面放大到400%不会自动增加成像信息。")
        ]
    }
    CodeRef { file: "examples/opencv/geometry/main.cpp"; region: "affine" }
    CodeRef { file: "examples/opencv/geometry/output.txt"; match: "INTER_NEAREST|INTER_LINEAR|INTER_CUBIC|无噪声"; caption: qsTr("已有旋转示例：两种数据上的评价") }
    Para { text: qsTr("已有输出中，带逐像素噪声的图旋转再转回，最近邻像素差0.69小于线性的1.60；但无噪声方块与理想旋转比较，线性0.56优于最近邻0.76。逐像素误差偏好保留原噪声，不等于边缘位置更准。先选评价目标，再解释插值结果。") }
    Pitfall { text: qsTr("阈值、模糊、重采样、边界模型和标定误差都可能移动边缘。抛物线拟合有小数位也可能存在系统偏差；无有效峰、分母接近零等情况应作为测量无效处理，不把任何返回数值都视为可靠尺寸。") }
    Try { task: qsTr("两个方法都显示60.10，一个在重复观测中变化很小，能否由此断言它更接近真实60.00？像素差更小能否直接证明测量更准？"); answerNote: qsTr("变化小表示该条件下重复性较好，仍可能有稳定偏差。必须与已知尺寸或可信参照比较。像素差与几何定位误差衡量不同对象，不能互相替代。") }
    InSystem { text: qsTr("复用measure、geometry与calib3d的既有代码和输出，不重新生成图像。工位仍按像素量半径与偏心，本课没有把它包装成毫米级测量设备。") }
}
