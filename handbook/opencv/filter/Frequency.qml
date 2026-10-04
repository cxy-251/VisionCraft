import QtQuick
import VisionCraft

Section {
    title: qsTr("频域滤波")
    lead: qsTr("傅里叶变换把图像拆成许多不同频率、不同方向的正弦波。周期性的干扰（条纹、网纹）在频谱里集中成一两个亮点，把它们抹掉再变换回去，干扰就没了，其余的内容几乎不受影响——这是普通模糊做不到的。")

    Why {
        text: qsTr("模拟工件的传送带有一种周期约 18 行的横条纹。用模糊去掉它，垫圈也一起糊了；而在频域里，条纹只占据很少几个频率，可以「精准」地去掉。示例找出条纹的频率，做一个陷波滤波，再和高斯模糊比较。")
    }

    CodeRef { file: "examples/opencv/freq_codes/main.cpp"; region: "dft" }
    CodeRef { file: "examples/opencv/freq_codes/output.txt"; from: "==== 1"; to: "周期 18.0"; caption: qsTr("实际输出") }
    KeyPoints {
        points: [
            qsTr("dft 的结果和原图一样大，每个点是一个复数：它的位置 (u, v) 表示频率和方向，大小（magnitude）表示这个正弦波有多强。"),
            qsTr("条纹只沿竖直方向变化，所以落在频谱的第 0 列；图高 360 行、周期 18 行，一共 20 个周期，所以在第 20 行——找到的峰正是这里。生成代码里的 sin(y × 0.35) 周期 2π / 0.35 ≈ 17.95 行，对得上。"),
            qsTr("实数图像的频谱是对称的：第 20 行的峰在第 360 − 20 = 340 行还有一个，陷波时两个都要去掉。")
        ]
    }

    CodeRef { file: "examples/opencv/freq_codes/main.cpp"; region: "notch" }
    CodeRef { file: "examples/opencv/freq_codes/main.cpp"; region: "blur-compare" }
    CodeRef { file: "examples/opencv/freq_codes/output.txt"; from: "传送带条纹起伏"; to: "对比"; caption: qsTr("实际输出") }
    Figure {
        files: ["handbook/opencv/figures/freq-input.png", "handbook/opencv/figures/freq-spectrum.png", "handbook/opencv/figures/freq-notched.png"]
        captions: [qsTr("原图：传送带有横条纹"), qsTr("频谱（取对数、零频移到中心）：中心上下一对亮点就是条纹"), qsTr("陷波后：条纹基本消失，垫圈和划痕不变")]
    }
    KeyPoints {
        label: qsTr("读结果")
        points: [
            qsTr("陷波：条纹起伏从 11.3 降到 3.3，垫圈区域平均只变了 2.56——绝大部分只是噪声的细微变化。"),
            qsTr("高斯模糊（sigma 6）：条纹只降到 5.1，不如陷波干净；垫圈区域却平均变了 14.37，边缘和划痕都被抹平了。")
        ]
    }

    Pitfall {
        text: qsTr("陷波去掉的是「这个频率」，不是「传送带区域」：如果垫圈表面恰好也有同样周期、同样方向的纹理，会被一起去掉。另外，频谱里只有条纹「恰好在图里重复整数次」时才是一个干净的点："
                 + "这里周期 17.95 行，360 行里是 20.06 个周期，不是整数，能量会漏到相邻的频率上；条纹又在垫圈处被挡住、断开，也会让它散开。示例剩下的 3.3 应该来自这两点，我没有把两者分开量过。")
    }

    Try {
        task: qsTr("如果传送带的条纹改成斜的（比如 45°），频谱里的亮点会出现在哪里？陷波的位置要怎么改？")
        answerNote: qsTr("亮点会离开第 0 列，出现在频谱的对角线方向上：频率向量 (u, v) 的方向就是条纹变化最快的方向，也就是垂直于条纹的方向。"
                       + "陷波时不能再写死在第 0 列，要先在频谱里找峰（像示例找 peakRow 那样，但在二维范围里找），再在峰和它的对称点周围置零。")
    }

    InSystem {
        text: qsTr("工位检测用黑帽找表面缺陷，黑帽本身对横条纹不敏感（条纹在垫圈外面，而且被环面掩膜挡住了），所以没有用频域滤波。数据由 examples/opencv/freq_codes 测得。")
    }
}
