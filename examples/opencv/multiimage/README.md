# 多图成像数值示例

构建目标 `example_multiimage_lessons`。运行时不带参数只打印数值；传入输出目录则保存固定输入和结果PNG。`bash examples/opencv/multiimage/generate_assets.sh`从仓库根路径生成手册素材及output.txt。

案例0–6使用种子20261017的480×240场景，两幅320×240观测重叠160列。颜色解码/编码采用gamma2.2教学约定；掩膜独立于像素值。案例3是受控局部错位，不冒充物理三维渲染；真正两深度投影见integration/projection。案例7从线性高动态场景生成三种曝光，HDR使用已知响应，不声称估计了相机响应。

共享实现位于src/vision/teaching/MultiImageOps，应用手册按需演示调用同一函数。输出来自一次数值示例运行，没有界面截图或硬件操作。
