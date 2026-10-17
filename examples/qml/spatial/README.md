# 三维场景示例

源码选配示例，正文在应用手册「Qt → 三维场景」。主安装包不包含此可执行程序。

使用与 Qt 版本匹配的 Quick3D 开发组件（本机为6.9.1），配置 `-DVC_BUILD_EXAMPLES=ON -DVC_BUILD_SPATIAL_EXAMPLE=ON`，构建 `example_spatial_lessons`，运行 `build/examples/qml/spatial/example_spatial_lessons`。

六个模式共用场景与参数。点击物体后横向拖动改变Y旋转；材质可切换棋盘纹理、无光照与透明度。没有自动动画。右侧是共享ProjectionMath的固定640×480针孔投影，不是GPU读回；视口比例、薄板厚度和模型差异详见正文。

Quick3D依赖的授权与仓库自身MIT声明分别处理，参见官方模块文档。此目标不进入默认发行包。构建通过不代表已验证所有平台的图形后端。
