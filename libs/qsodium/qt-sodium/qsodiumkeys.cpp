#include "qsodiumkeys.h"


bool QSodiumKeys::saveKeys(QString outDir, const QString &pubName, const QString &priName)
{

    std::filesystem::path const path = outDir.toStdString();

    // Extract just the filename components ("identity.pub", "identity.key")
    std::string const pubPath = QFileInfo(pubName).fileName().toStdString();
    std::string const priPath = QFileInfo(priName).fileName().toStdString();

    return m_keys->saveKeys(path, pubPath, priPath);
}



bool QSodiumKeys::loadKeysFromDisk(const QString &pubName, const QString &priName) noexcept
{
    std::filesystem::path pubPath = pubName.toStdString();
    std::filesystem::path priPath = priName.toStdString();
    return m_keys->loadKeysFromDisk(pubPath, priPath);
}

