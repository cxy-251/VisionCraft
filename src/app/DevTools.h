#pragma once
#include <QObject>
#include <QtQml/qqmlregistration.h>
class DevTools : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString output READ output NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString systemInfo READ systemInfo NOTIFY systemInfoChanged)
public:
    using QObject::QObject;
    QString output()const{return m_output;}
    QString status()const{return m_status;}
    QString systemInfo()const;
    Q_INVOKABLE void refreshSystemInfo() { emit systemInfoChanged(); }
    Q_INVOKABLE void formatJson(const QString &text,bool compact);
signals:
    void changed();
    void systemInfoChanged();
private:QString m_output,m_status;
};
