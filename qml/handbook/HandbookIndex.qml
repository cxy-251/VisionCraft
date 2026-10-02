import QtQml

// 手册目录的根：handbook/Index.qml 写成 HandbookIndex { Volume { ... } ... }
QtObject {
    default property list<QtObject> volumes
}
