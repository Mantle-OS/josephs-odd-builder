#pragma once

#include <QStringListModel>
#include <QQmlEngine>
#include <QObject>

class QmlStringList : public QStringListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use from C++ and expose as a model to QML")

public:
    explicit QmlStringList(QObject *parent = nullptr);
    ~QmlStringList()
    {
        if(!isEmpty())
            clear();
    }

public Q_SLOTS:
    void setStringList(const QStringList &list);

    int count() const;
    int size() const;
    int length() const;
    bool isEmpty() const;
    void prepend(const QString &value);
    void append(const QString &value);
    void push_back(const QString &value);
    void push_front(const QString &value);
    bool move(qsizetype, qsizetype);
    void clear();
    bool removeAt(qsizetype);
    int indexOf(const QString &value) const;
    QString at(int index) const;

    bool contains(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    qsizetype lastIndexOf(QLatin1StringView str, qsizetype from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    qsizetype removeDuplicates();

    QString takeFirst() const;
    QString takeLast() const;

};
