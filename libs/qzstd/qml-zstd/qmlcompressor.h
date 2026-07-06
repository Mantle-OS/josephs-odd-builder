#pragma once
#include "qzstdcompressor.h"
#include "qzstdoptions.h"
#include <qqmlintegration.h>
class QmlCompressor : public QZstdOptions
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit QmlCompressor(QObject *parent = nullptr):
        QZstdOptions{parent}
    {
        QZstdOptions    *opts   = dynamic_cast<QZstdOptions*>(this);
        m_comp = new QZstdCompressor{opts};
    }

    ~QmlCompressor()
    {
        delete m_comp;
        m_comp = nullptr;
    }
    Q_INVOKABLE bool compress()     { return m_comp->compress();}

private:
    QZstdCompressor         *m_comp;
};