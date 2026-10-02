#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

// 手册读取「真实文件」的唯一入口：正文 QML、示例代码、本项目源码。
//
// 发布模式下文件来自编译进程序的资源（:/handbook/...、:/examples/...、:/src/...）；
// 开发模式（环境变量 VC_DEV=1）直接读源码目录，并在文件保存时发出 fileChanged，
// 手册页据此重新加载，改完不用重新编译。
class SourceProvider : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool devMode READ devMode CONSTANT)
    Q_PROPERTY(QString monoFont READ monoFont CONSTANT)

public:
    explicit SourceProvider(QObject *parent = nullptr);

    bool devMode() const { return m_devMode; }
    QString monoFont() const;

    // path 一律相对于项目根目录，例如 "examples/qt/signals_slots/main.cpp"
    Q_INVOKABLE QString read(const QString &path) const;

    // 取出 `// [region name]` 与 `// [endregion]` 之间的代码，去掉公共缩进
    Q_INVOKABLE QString region(const QString &path, const QString &name) const;

    // region 第一行在原文件中的行号（从 1 开始），找不到返回 0
    Q_INVOKABLE int regionLine(const QString &path, const QString &name) const;

    // 手册正文的加载地址
    Q_INVOKABLE QUrl contentUrl(const QString &path) const;

    // 开发模式下监视文件，保存后发出 fileChanged(path)
    Q_INVOKABLE void watch(const QString &path);

    // 重新加载正文前调用：清掉引擎缓存的已编译组件，否则加载到的还是旧版本
    Q_INVOKABLE void clearCache();

signals:
    void fileChanged(const QString &path);

private:
    QString diskPath(const QString &path) const;

    bool m_devMode = false;
    QFileSystemWatcher m_watcher;
};
