# 六节补充：输入语义与结果解释

目标：依据真实目录、正文、独立示例和旧知识注册代码，补充尚未系统讲解的主题。教学示例保持独立，正文进入应用QML手册；不增加业务工具或下位机功能。

## 缺口依据

- ROI共享、clone、step：MatMemory、ImageArray、ImageDelivery已有。
- 掩膜、均值和copyTo：Masks已有；直方图已有阈值演示和专章。
- 坐标变换：GraphicsView/Painter/Input与ImageView已有。
- Canvas/QQuickPaintedItem：Painter已有对照；混叠：Pyramid与Scope已有。
- 焦点在Events中仅列为事件类型，QShortcut仅被提及，缺少Qt Quick作用域与隐藏页快捷键的专门讲解。
- 原Input没有表单草稿/Validator提交和Accessible；OpenCV源码与课程未找到kmeans、IoU/Dice、MSE/PSNR专门材料。

## 完成内容

- [x] Qt Quick焦点、KeyNavigation与窗口快捷键。
- [x] 编辑中间态、整数校验、草稿与明确提交。
- [x] Accessible语义、键盘与辅助动作的共同入口。
- [x] K-means样本布局、中心、初始标签、SSE和语义边界。
- [x] 二值前景IoU/Dice、像素准确率与空样本约定。
- [x] MSE/PSNR峰值与相同总误差下的不同空间分布。
- [x] 两个独立教学目标与固定数据实际输出。
- [x] 应用目录、学习路线、综合练习与维护地图衔接。

## 入口与范围

Qt新增三节在“Qt → 界面：QML”；OpenCV新增三节在“OpenCV → 聚类与结果评价”。系统综合练习追加四题与答案。Qt现44节（QML12节），OpenCV54节。

- `example_qml_forms`：三个独立教学页，从源码加载QML。
- `example_result_interpretation`：固定八个颜色、100×100掩膜与8×8误差；output.txt由示例本次运行生成，未使用外部图片或屏幕取图。

QML交互留给读者主动操作，不自动打开窗口，不声称读屏软件或全部键盘环境已经验收。必要检查为示例/主应用构建及新增、变更QML字节码编译；不新增或运行测试，不保存截图。整批完成后一次本地提交，不推送。

完成记录：两个教学目标和VisionCraft主应用构建通过；13份新增/变更QML字节码编译通过。固定数据示例运行一次，产生所引用的output.txt。未自动运行QML交互窗口或使用辅助软件。
