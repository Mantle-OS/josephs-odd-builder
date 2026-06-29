#ifndef QZSTDCOMPRESSOR_H
#define QZSTDCOMPRESSOR_H

#include "qzstdoptions.h"

class QZstdCompressor : public QZstdOptions
{
    Q_OBJECT
public:
    explicit QZstdCompressor(QObject *parent = nullptr);
    ~QZstdCompressor() override = default;

public Q_SLOTS:
    virtual bool execute();
    virtual bool compressFolder();
    virtual bool compressFile();
};
#endif // QZSTDCOMPRESSOR_H