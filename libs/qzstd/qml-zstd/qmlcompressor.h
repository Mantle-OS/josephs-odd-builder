#pragma once
#include "qzstdcompressor.h"
#include "qzstdoptions.h"
#include <qqmlintegration.h>

#include "qmlzstd_export.h"
class QMLZSTD_EXPORT QmlCompressor : public QZstdOptions
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit QmlCompressor(QObject *parent = nullptr):
        QZstdOptions{parent},
        m_comp{ new QZstdCompressor{this}}
    {
    }

    ~QmlCompressor()
    {
        if(m_comp){
            delete m_comp;
            m_comp = nullptr;
        }
    }
    Q_INVOKABLE bool compress()     { return m_comp->compress();}

private:
    QZstdCompressor         *m_comp;
};