#pragma once

#include <QObject>
#include <qqmlregistration.h>

#include <qt-zstd/qzstd.h>
#include <qt-zstd/qzstdcompressor.h>
#include <qt-zstd/qzstddecompressor.h>

class QmlZstd : public QZstd
{
    Q_OBJECT
    Q_PROPERTY(bool hasSodium READ hasSodium NOTIFY hasSodiumChanged FINAL)

    QML_ELEMENT
    QML_SINGLETON
public:
    explicit QmlZstd(QObject *parent = nullptr) :
        QZstd{parent}
    {

    }
    ~QmlZstd() override = default;

#ifdef QZSTD_SODIUM_SUPPORT
    bool hasSodium() const
    {
        return true;
    }
#else
    bool hasSodium() const
    {
        return false;
    }
#endif

Q_SIGNALS:
    void hasSodiumChanged();

private:
#ifdef QZSTD_SODIUM_SUPPORT
    bool m_hasSodium = true;
#else
    bool m_hasSodium = false;
#endif
};


class QmlCompressor : public QZstdCompressor
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit QmlCompressor(QObject *parent = nullptr) :
        QZstdCompressor(parent)
    {}
    ~QmlCompressor() override = default;

    Q_INVOKABLE bool compress() { return execute();}
};

class QmlDecompressor : public QZstdDecompressor
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit QmlDecompressor(QObject *parent = nullptr) :
        QZstdDecompressor(parent) {}
    ~QmlDecompressor() override = default;

    Q_INVOKABLE bool decompress() { return execute();}
};
