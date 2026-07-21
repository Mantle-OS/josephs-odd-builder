#pragma once
#include <qqmlintegration.h>
#include <qzstddecompressor.h>
#include <qzstdoptions.h>
#include "qmlzstd_export.h"
class QMLZSTD_EXPORT QmlDecompressor : public QZstdOptions
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit QmlDecompressor(QObject *parent = nullptr) :
        QZstdOptions{parent},
        m_dec{new QZstdDecompressor{this}}
    {
    }
    ~QmlDecompressor()
    {
        if(m_dec){
            delete m_dec;
            m_dec = nullptr;
        }
    }
    Q_INVOKABLE bool decompress() { return m_dec->decompress();}

private:
    QZstdDecompressor *m_dec = nullptr;
};
