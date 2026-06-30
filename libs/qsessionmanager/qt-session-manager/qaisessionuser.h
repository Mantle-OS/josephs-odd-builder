#ifndef QAISESSIONUSER_H
#define QAISESSIONUSER_H

#include <QObject>

#include <aisession/session_user.hpp>

using namespace job::serializer::generated;
class QAiSessionUser : public QObject
{
    Q_OBJECT
public:
    explicit QAiSessionUser(QObject *parent = nullptr);

private:
    AiSessionUser m_user;
    // std::string user_id;
    // std::string display_name;
    // std::string icon;
    // // Path to encrypted/compressed private vault.
    // std::string vault_path;
    // // Argon2id modular crypt string from crypto_pwhash_str.
    // std::string password_hash;
    // // ARGON2ID13=0
    // uint32_t kdf_alg;
    // // crypto_pwhash_SALTBYTES salt used to derive the vault key.
    // std::array<uint8_t, 16> kdf_salt;
    // std::string created_at;
    // std::string last_login;

};

#endif // QAISESSIONUSER_H
