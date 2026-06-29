#ifndef QZSTDDECOMPRESSOR_H
#define QZSTDDECOMPRESSOR_H

#include "qzstdoptions.h"

class QZstdDecompressor : public QZstdOptions
{
    Q_OBJECT
public:
    explicit QZstdDecompressor(QObject *parent = nullptr);
    ~QZstdDecompressor() override = default;

public Q_SLOTS:
    virtual bool execute();
    virtual bool decompressFolder();
    virtual bool decompressFile();
};

#endif // QZSTDDECOMPRESSOR_H