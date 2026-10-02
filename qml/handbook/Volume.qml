import QtQml

// 手册目录：卷。在 handbook/Index.qml 里声明。
QtObject {
    property string key
    property string title
    property string summary
    default property list<QtObject> chapters
}
