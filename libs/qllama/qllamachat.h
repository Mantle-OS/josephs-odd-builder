#ifndef QLLAMACHAT_H
#define QLLAMACHAT_H

#include <QObject>
#include <QQmlEngine>

class QLlamaChat : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit QLlamaChat(QObject *parent = nullptr);








};

#endif // QLLAMACHAT_H
