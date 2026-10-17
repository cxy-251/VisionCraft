# 坐标与投影数值示例

构建 `example_projection_lesson`。无参数打印坐标链与零旋转视差数值；传入目录保存几何投影图。`generate_assets.sh`生成手册素材与output.txt。

共享ProjectionMath采用毫米、640×480像素、主点(319.5,239.5)。Qt相机的局部方向显式转换到OpenCV；相机朝向固定，物体可绕Y轴旋转。数值图没有GPU读回、镜头畸变或遮挡模拟。图内两个边框的间距不等于两次相机运动之间的视差残差。
