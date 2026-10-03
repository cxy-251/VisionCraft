import QtQml

QtObject {
    // [region loop]
    property int a: b + 1
    property int b: a + 1                              // a 依赖 b，b 又依赖 a
    // [endregion]
}
