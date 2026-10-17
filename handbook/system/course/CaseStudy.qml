import QtQuick
import VisionCraft
Section {
    title: qsTr("综合案例：从问题定义到方法选择")
    lead: qsTr("同一张固定场景，目标是找出两个带规定结构的亮区域。先写出已知条件，再逐步比较区域提取、候选清理、数量与孔洞；每一步都解释自己使用了哪些信息。")
    Why { text: qsTr("方法对照讲各方法差异，这里把它们组合成一份可推理的解题过程。程序画出亮度渐变背景、两个矩形区域、一个孔和一个孤立亮点；真值只用于说明结果，不作为待比较方法的答案输入。") }
    CodeRef { file: "examples/opencv/teaching_case/main.cpp"; region: "scene" }
    KeyPoints {
        label: qsTr("题目与前提")
        points: [
            qsTr("背景从左到右逐渐变亮，目标比当地背景亮60；一个目标有孔，孤立亮点不属于目标。任务需要填充区域、目标数量和孔洞信息，不是只寻找亮度边缘。"),
            qsTr("全局阈值、自适应阈值只拿输入图；背景差分分支另外获得一张完全对齐的已知背景。后者条件更强，必须明确说出，不能把有额外参照的结果当作算法无条件胜出。"),
            qsTr("生成时使用truth来建立场景，评价时用它计数；提取方法没有拿truth选像素。区分数据生成、处理输入与评价答案，才能避免把答案泄漏给方法。")
        ]
    }
    CodeRef { file: "examples/opencv/teaching_case/main.cpp"; region: "methods" }
    CodeRef { file: "examples/opencv/teaching_case/main.cpp"; region: "area" }
    CodeRef { file: "examples/opencv/teaching_case/output.txt"; caption: qsTr("固定案例的实际结果；不是通用方法排名") }
    KeyPoints {
        label: qsTr("按问题解释各列")
        points: [
            qsTr("inside/outside/missed分别表达选中真实目标、误选非目标和漏掉目标的像素；objects按8连通计数，childContours报告本例层级中的内轮廓。区域像素对了多少与物体数、孔洞是否正确要分别读。"),
            qsTr("差分后孤立亮点仍会留下：它相对背景也发生了变化。开运算可以按形状尺度清理，按连通域面积≥100也可以清理；它们使用了不同的保留规则。"),
            qsTr("面积过滤保留足够大的已有区域，不像形态学那样直接改动每个保留区域的边界；代价是会拒绝所有小于阈值的真实对象。目标最小尺度属于先验条件。"),
            qsTr("Canny输出只报告边界点，不拿它与填充区域的truth做相同计分；若后续要由边缘构造区域，需要额外说明闭合、填充和孔洞规则。")
        ]
    }
    Para { text: qsTr("本例真值2395像素。全局阈值漏991、误选2485；局部阈值与参照差分都完整保留目标，但多出同一个亮点。开运算和面积过滤在本例都得到2395像素、2个目标、1条内轮廓。结果相同只说明这一输入下相同，不取消两种清理规则的差别。") }
    Pitfall { text: qsTr("这个场景提供理想参照与明确尺度，结果不能外推成任意光照、任意相机上的效果承诺。参照没对齐、目标不再比背景亮60、细结构尺度变化时，应先重新检查前提，不急着继续加后处理。") }
    Try { task: qsTr("如果目标比背景只亮20，差分阈值仍为30会怎样？把一个真实目标缩小到50像素，面积过滤还适合原任务吗？"); answerNote: qsTr("目标差异低于30就会被排除，后处理不能凭空找回已消失的候选；50像素目标会被≥100规则删除。二者都是前提变化，应调整方法定义和参数，而不是把漏掉的目标解释成没有目标。") }
    InSystem { text: qsTr("目标example_teaching_case，运行build/examples/opencv/teaching_case/example_teaching_case。它是数值课堂案例，不接设备、不增加实用检测工具；与方法对照、标样对齐、连通域和轮廓章节对读。") }
}
