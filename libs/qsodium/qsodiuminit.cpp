#include "qsodiuminit.h"
#include <QDebug>
bool QSodiumInit::initialize() noexcept
{
    if (s_initialized)
        return true;

    if (sodium_init() < 0) {
        qWarning() << "[QSodiumInit] libsodium initialization failed!";
        return false;
    }

    s_initialized = true;
    qDebug() << "[QSodiumInit] libsodium initialized.";
    return true;
}

bool QSodiumInit::isInitialized() noexcept
{
    return s_initialized;
}