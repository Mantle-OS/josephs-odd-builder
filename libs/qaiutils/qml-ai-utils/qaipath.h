#pragma once

#include <QObject>
#include <QQmlEngine>

#include "property-macros.h"
#include "pointer-macros.h"
#include "qmlstringlist.h"
#include "qmlaiutils_export.h"

// VERY ALPHA SUBJECT TO CHANGE !!!!!!!!!
class QMLAIUTILS_EXPORT QAiPath : public QObject
{
    Q_OBJECT
    QP_RW(QString,              currentModel,           ""          )
    QP_PTR_RO(QmlStringList,    modelDirs                           ) // NON owned for now. will come back later. [[Depends on session manager]]
    QP_PTR_RO(QmlStringList,    availableModels                     ) // NON owned for now. will come back later. [[Depends on session manager]]
    QML_ELEMENT
public:
    explicit QAiPath(QObject *parent = nullptr);
};

