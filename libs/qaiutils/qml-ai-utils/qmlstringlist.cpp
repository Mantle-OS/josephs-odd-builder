#include "qmlstringlist.h"

QmlStringList::QmlStringList(QObject *parent) :
    QStringListModel(parent)
{
}

void QmlStringList::setStringList(const QStringList &list)
{
    beginResetModel();
    QStringListModel::setStringList(list);
    endResetModel();
}

void QmlStringList::append(const QString &value)
{
    const int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    QStringList list = stringList();
    list.append(value);
    QStringListModel::setStringList(list);
    endInsertRows();
}

void QmlStringList::push_back(const QString &value)
{
    append(value);
}

void QmlStringList::push_front(const QString &value)
{
    beginInsertRows(QModelIndex(), 0, 0);
    QStringList list = stringList();
    list.prepend(value);
    setStringList(list);
    endInsertRows();
}

bool QmlStringList::move(qsizetype from, qsizetype to)
{
    if (from < 0 || from >= rowCount() || to < 0 || to >= rowCount() || from == to)
        return false;

    QStringList list = stringList();
    list.move(from, to);
    setStringList(list);
    return true;
}

void QmlStringList::clear()
{
    beginResetModel();
    QStringListModel::setStringList({});
    endResetModel();
}

bool QmlStringList::removeAt(qsizetype index)
{
    if (index < 0 || index >= rowCount())
        return false;

    beginRemoveRows(QModelIndex(), index, index);
    QStringList list = stringList();
    list.removeAt(index);
    setStringList(list);
    endRemoveRows();
    return true;
}

int QmlStringList::indexOf(const QString &value) const
{
    return stringList().indexOf(value);
}

QString QmlStringList::at(int index) const
{
    const QStringList list = stringList();
    return (index >= 0 && index < list.size()) ? list.at(index) : QString{};
}

bool QmlStringList::contains(const QString &str, Qt::CaseSensitivity cs) const
{
    return stringList().contains(str, cs);
}
qsizetype QmlStringList::lastIndexOf(QLatin1StringView str, qsizetype from, Qt::CaseSensitivity cs) const
{
    return stringList().lastIndexOf(str, from, cs);
}
qsizetype QmlStringList::removeDuplicates()
{
    QStringList list = stringList();
    qsizetype oldSize = list.size();
    list.removeDuplicates();
    setStringList(list);
    return oldSize - list.size();
}


QString QmlStringList::takeFirst() const
{
    if (isEmpty())
        return QString();

    QStringList list = stringList();
    QString value = list.takeFirst();
    const_cast<QmlStringList *>(this)->setStringList(list);
    return value;
}

QString QmlStringList::takeLast() const
{
    if (isEmpty())
        return QString();

    QStringList list = stringList();
    QString value = list.takeLast();
    const_cast<QmlStringList *>(this)->setStringList(list);
    return value;
}

int QmlStringList::count() const
{
    return rowCount();
}

int QmlStringList::size() const
{
    return count();
}

int QmlStringList::length() const
{
    return count();
}

bool QmlStringList::isEmpty() const
{
    return rowCount() == 0 ? true : false;
}

void QmlStringList::prepend(const QString &value)
{
    push_front(value);
}
