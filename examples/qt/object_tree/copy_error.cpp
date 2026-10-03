// QObject 不能拷贝：它有身份（父子关系、连接、对象名），复制出一个「一模一样的」没有意义
#include <QObject>

void f()
{
    QObject a;
    QObject b = a;
}
