#ifndef QMLSODIUMKEYS_H
#define QMLSODIUMKEYS_H

#include <QObject>
#include <QQmlEngine>

#include <property-macros.h>
#include <qsodiumkeys.h>
#include <qaiutils.h>

class QmlSodiumKeys : public QObject
{
    Q_OBJECT

    QP_RW(QString, keyDir, "")
    QP_RW(QString, publicKeyFile, "")
    QP_RW(QString, privateKeyFile, "")
    QP_RW(QString, publicKeyBase64, "")

    QML_ELEMENT
public:
    enum class KeyType {
        Exchange = static_cast<int>(QSodiumKeys::KeyType::Exchange),
        Sign     = static_cast<int>(QSodiumKeys::KeyType::Sign)
    };
    Q_ENUM(KeyType)

    explicit QmlSodiumKeys(QObject *parent = nullptr) :
        QObject{parent},
        m_keys{new QSodiumKeys{}}
    {
        // Safe value capture of the 'this' object instance context pointer
        connect(this, &QmlSodiumKeys::publicKeyBase64Changed, this, [this](const QString &key){
            if (m_keys)
                m_keys->setPublicKey(key);
        });
    }

    ~QmlSodiumKeys() override {
        delete m_keys;
        m_keys = nullptr;
    }

    Q_INVOKABLE bool create(QmlSodiumKeys::KeyType type) noexcept;
    Q_INVOKABLE bool validSet() noexcept { return m_keys && m_keys->isValid(); }

    Q_INVOKABLE bool loadKeysFromDisk() noexcept;
    Q_INVOKABLE bool saveKeysToDisk() noexcept;

private:
    QSodiumKeys *m_keys = nullptr;
    QString getFullPath(const QString &fileName) const noexcept;
};

#endif // QMLSODIUMKEYS_H