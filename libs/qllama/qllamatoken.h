#ifndef QLLAMATOKEN_H
#define QLLAMATOKEN_H

#include <QObject>
#include <QQmlEngine>
#include <llama.h>
class QLlamaToken : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit QLlamaToken(QObject *parent = nullptr);

private:
    llama_token *m_token;
};

#endif // QLLAMATOKEN_H
