#ifndef QAIUTILS_H
#define QAIUTILS_H

#include <QObject>
#include <QHash>
#include <QString>

#include "qaiutils_export.h"
class QAIUTILS_EXPORT QAiUtils {
public:
    static QAiUtils &instance();

    static QString appName;
    static QString baseDir;

    static QString configsDir;
    static QString jsonConfigsDir;
    static QString yamlConfigsDir;

    static QString appRuntimeDir;
    static QString logsDir;
    static QString outDir;

    static QString modelsDir;
    static QString checkpointsDir;
    static QString diffusionDir;
    static QString textEncoderDir;
    static QString lorasDir;
    static QString embeddingsDir;
    static QString controlNetDir;
    static QString upscaleModelDir;
    static QString vaeDir;
    static QString audioVaeDir;

    static QString packagesDir;
    static QString pluginsDir;
    static QString extraQmlDir;

    static QString userDir;
    static QString userJson;

    static void debugPaths();
    static bool createDefaultDirs();
    static bool createDirFromFile(const QString &fileName);
    static bool createDir(const QString &dirName);
    static bool writeTextFile(const QString &fileName, const QString &content);
    static QString readTextFile(const QString &fileName);
    static bool fileExists(const QString &filePath);
    static bool dirExists(const QString &dirPath);
    // static QString hashFile(const QString &filePath, QCryptographicHash::Algorithm algo = QCryptographicHash::Md5);


    QAiUtils(const QAiUtils&) = delete;
    QAiUtils &operator=(const QAiUtils&) = delete;

private:
    QAiUtils();

};
#endif // QAIUTILS_H
