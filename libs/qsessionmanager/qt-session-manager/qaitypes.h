#ifndef QAITYPES_H
#define QAITYPES_H

#include <QObject>

namespace QAi {
Q_NAMESPACE

enum class Provider : uint32_t {
    HuggingFace = 0,
    OpenAI      = 1,
    GitHub      = 2,
    Local       = 3
};
Q_ENUM_NS(Provider)

enum class KeyKind : uint32_t {
    Identity       = 0,
    PackageSigning = 1,
    LedgerSigning  = 2
};
Q_ENUM_NS(KeyKind)

enum class CredentialKind : uint32_t {
    BearerToken = 0,
    ApiKey      = 1,
    Basic       = 2
};
Q_ENUM_NS(CredentialKind)
}

#endif // QAITYPES_H