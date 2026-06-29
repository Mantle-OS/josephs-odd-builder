#ifndef QAIPATH_H
#define QAIPATH_H

#include <QObject>
#include <QQmlEngine>

#include "property-macros.h"
#include "pointer-macros.h"
#include "qmlstringlist.h"

class QAiPath : public QObject
{
    Q_OBJECT
    QP_RW(QString,              currentModel,           "" )
    QP_PTR_RO(QmlStringList,    modelDirs)
    QP_PTR_RO(QmlStringList,    availableModels)
    QML_ELEMENT
public:
    explicit QAiPath(QObject *parent = nullptr) :
        QObject{parent}
    {

    }
};

#endif // QAIPATH_H
