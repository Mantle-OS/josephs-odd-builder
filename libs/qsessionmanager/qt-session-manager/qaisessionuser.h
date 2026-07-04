#ifndef QAISESSIONUSER_H
#define QAISESSIONUSER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <property-macros.h>
#include <aisession/session_user.hpp>
namespace jsgen = job::serializer::generated;
class QAiSessionUser : public QObject
{
    Q_OBJECT

    // Map your property getters to match the underlying fields
    QP_RO(QString, userId, "")
    QP_RO(QString, displayName, "")
    QP_RO(QString, icon, "")
    QP_RO(QString, vaultPath, "")

public:
    explicit QAiSessionUser(QObject *parent = nullptr);
    ~QAiSessionUser() noexcept override;

    QAiSessionUser(const QAiSessionUser &) = delete;
    QAiSessionUser &operator=(const QAiSessionUser &) = delete;

    void updateFromStruct(const jsgen::AiSessionUser &userStruct) noexcept;

    // Direct read-only accessors for internal crypto logic if needed by the manager
    [[nodiscard]] std::string passwordHash() const noexcept { return m_user.password_hash; }
    [[nodiscard]] std::array<uint8_t, 16> kdfSalt() const noexcept { return m_user.kdf_salt; }
    [[nodiscard]] uint32_t kdfAlg() const noexcept { return m_user.kdf_alg; }

private:
    jsgen::AiSessionUser m_user;
};

#endif // QAISESSIONUSER_H