import QtQuick
import VisionCraft
Section {
    title: qsTr("图像误差：MSE、PSNR与空间分布")
    lead: qsTr("两张图的平均误差相同，错误可能发生在完全不同的位置。一个数字能概括某种差异，也会丢掉其他信息。")
    Why { text: qsTr("已有像素测量误差、标样对齐与图像数组课程。本节新增成对图像误差指标；不把处理后更像原图，直接当作检测方法更好。") }
    CodeRef { file: "examples/opencv/interpretation/main.cpp"; region: "error" }
    KeyPoints {
        label: qsTr("先约定比较条件")
        points: [
            qsTr("示例限定同尺寸、单通道8位图，同一坐标代表同一个位置。先转CV_64F再相减，避免8位无符号减法把负差截成0。MSE是全部像素平方误差的平均值。"),
            qsTr("PSNR=10 log10(L²/MSE)，本例约定峰值L=255，单位dB。若用0–1浮点数据，应相应使用L=1；不能只换存储类型就沿用错误的峰值。"),
            qsTr("两图完全一致时MSE=0，PSNR趋向正无穷。它表示在这个精确比较条件下没有数值差异，不代表算法具有无限检测能力。"),
            qsTr("多通道比较还需约定逐通道还是整体平均、颜色空间和通道数量。本例用单通道避免隐含这些选择，不能直接把公式中的像素数当成任意多通道的样本数。")
        ]
    }
    CodeRef { file: "examples/opencv/interpretation/main.cpp"; region: "pairs" }
    CodeRef { file: "examples/opencv/interpretation/output.txt"; from: "^=== equal"; caption: qsTr("两种误差分布的实际输出") }
    Para { text: qsTr("8×8共64像素：全部像素加5，MSE=64×25/64=25；只改一个像素加40，MSE=1600/64=25。两者PSNR都约34.151404 dB，但一个是整体亮度偏移，一个是局部强变化。这个例子没有发生255饱和，平方误差可以直接手算。") }
    Pitfall { text: qsTr("去噪、增强和检测的目标不同。输出与带噪输入越接近，可能只是没有去噪；应使用与任务相符的参照。几何没对齐也会造成大误差，不能把PSNR下降直接解释为表面缺陷增加。MSE/PSNR没有描述错误位置、结构或感知质量。") }
    Try { task: qsTr("64像素中只改4个像素，每个加20，与上述两种情况的MSE是否相同？仅凭PSNR能区分它们吗？"); answerNote: qsTr("4×400/64=25，因此相同，PSNR也相同。需要查看误差分布或采用与实际任务相符的其他评价，不能靠同一个总量恢复被丢掉的空间信息。") }
    InSystem { text: qsTr("example_result_interpretation第三部分。与ReferenceAlignment、MeasurementError对读：像素差异、几何误差和业务判定不是同一种量。") }
}
