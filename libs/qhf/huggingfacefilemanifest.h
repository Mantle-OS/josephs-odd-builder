#ifndef HUGGINGFACEFILEMANIFEST_H
#define HUGGINGFACEFILEMANIFEST_H

#include <QObject>
#include <QQmlEngine>
#include <property-macros.h>

class HuggingFaceFileManifest : public QObject
{
    Q_OBJECT
    QP_RO(bool,     installed,  false   )
    QP_RO(QString,  rfilename,  ""      )
    QP_RO(QString,  outDir,     ""      )
    QML_ELEMENT
public:
    explicit HuggingFaceFileManifest(QObject *parent = nullptr);
    ~HuggingFaceFileManifest() = default;

};

#endif // HUGGINGFACEFILEMANIFEST_H
