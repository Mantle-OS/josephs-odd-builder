#include "qaiutils.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStringList>

QAiUtils &QAiUtils::instance() {
    static QAiUtils instance;
    return instance;
}

void QAiUtils::debugPaths(){
    qDebug() << "Base Dir:              " << QAiUtils::baseDir;

    qDebug() << "Configuration Dir:     " << QAiUtils::configsDir;
    qDebug() << "Json Configuration Dir:" << QAiUtils::jsonConfigsDir;
    qDebug() << "YAML Configuration Dir:" << QAiUtils::yamlConfigsDir;

    qDebug() << "App Runtime Dir:       " << QAiUtils::appRuntimeDir;
    qDebug() << "Logs Dir:              " << QAiUtils::logsDir;
    qDebug() << "Out Dir:               " << QAiUtils::outDir;

    qDebug() << "Models Dir:            " << QAiUtils::modelsDir;
    qDebug() << "Checkpoint Dir:        " << QAiUtils::checkpointsDir;
    qDebug() << "Unet Dir:              " << QAiUtils::diffusionDir;
    qDebug() << "Text Encoder Dir:      " << QAiUtils::textEncoderDir;
    qDebug() << "LoRA's Dir:            " << QAiUtils::lorasDir;
    qDebug() << "Embeddings Dir:        " << QAiUtils::embeddingsDir;
    qDebug() << "Control Net Dir:       " << QAiUtils::controlNetDir;
    qDebug() << "Upscale Dir:           " << QAiUtils::upscaleModelDir;
    qDebug() << "VAE Dir:               " << QAiUtils::vaeDir;
    qDebug() << "Audio VAE Dir:         " << QAiUtils::audioVaeDir;
    qDebug() << "Packages Dir:          " << QAiUtils::packagesDir;

    qDebug() << "Plugins Dir:           " << QAiUtils::pluginsDir;
    qDebug() << "Extra QML Dir:         " << QAiUtils::extraQmlDir;
}

bool QAiUtils::createDefaultDirs()
{
    QStringList const dirs{
        baseDir,
        configsDir,
        jsonConfigsDir,
        yamlConfigsDir,
        appRuntimeDir,
        logsDir,
        outDir,
        modelsDir,
        checkpointsDir,
        diffusionDir,
        textEncoderDir,
        lorasDir,
        embeddingsDir,
        controlNetDir,
        upscaleModelDir,
        vaeDir,
        audioVaeDir,
        packagesDir,
        pluginsDir,
        extraQmlDir,
        userDir
    };

    for (const QString &dir : dirs) {
        if (!createDir(dir))
            return false;
    }

    return true;
}

bool QAiUtils::createDirFromFile(const QString &fileName){
    bool ret = false;
    QFileInfo fi(fileName);
    QDir d = fi.absoluteDir();
    if(!d.exists()){
        if(d.mkpath(d.absolutePath())){
            ret = true;
        }else{
            // LOG
        }
    }else{
        // we already have the dir return true
        ret = true;
    }
    return ret;
}

bool QAiUtils::createDir(const QString &dirName){
    bool ret = false;

    QDir d (dirName);
    if(!d.exists()){
        if(d.mkpath(dirName)){
            ret = true;
        }else{
            // LOG
        }
    }else{
        // we already have the dir return true
        ret = true;
    }
    return ret;
}

bool QAiUtils::writeTextFile(const QString &fileName, const QString &content)
{
    bool ret = false;

    if (QAiUtils::createDirFromFile(fileName)) {
        QFile f(fileName);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << content;
            if (f.flush())
                ret = true;
            else
                qWarning() << "Failed to flush the file buffer:" << f.errorString();
            f.close();
            if (f.error() != QFile::NoError) {
                qWarning() << "Error after closing the file:" << f.errorString();
                ret = false;
            }
        } else {
            qWarning() << "Failed to open file for writing:" << f.errorString();
        }
    } else {
        qWarning() << "Failed to create directory for file:" << fileName;
    }
    return ret;
}


QString QAiUtils::readTextFile(const QString &fileName)
{
    QString ret;
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        ret = in.readAll();
        f.close();
    }else{
        // log
    }
    return ret;
}

bool QAiUtils::fileExists(const QString &filePath) {
    return QFile::exists(filePath) && QFileInfo(filePath).isFile();
}

bool QAiUtils::dirExists(const QString &dirPath) {
    return QFile::exists(dirPath) && QFileInfo(dirPath).isDir();
}

QAiUtils::QAiUtils()
{
    appName                 = "qtai";
    baseDir                 = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).append("/" + appName);

    configsDir              = QString("%1/configs").arg(QAiUtils::baseDir);
    jsonConfigsDir          = QString("%1/json").arg(QAiUtils::configsDir);
    yamlConfigsDir          = QString("%1/yaml").arg(QAiUtils::configsDir);

    appRuntimeDir           = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).append("/" + appName);
    outDir                  = QString("%1/out").arg(QAiUtils::appRuntimeDir);
    logsDir                 = QString("%1/logs").arg(QAiUtils::appRuntimeDir);

    modelsDir               = QStandardPaths::writableLocation(QStandardPaths::CacheLocation).append("/" + appName + "/models");
    checkpointsDir          = QString("%1/checkpoints").arg(QAiUtils::modelsDir);
    diffusionDir            = QString("%1/unet").arg(QAiUtils::modelsDir);
    textEncoderDir          = QString("%1/textEncoders").arg(QAiUtils::modelsDir);
    lorasDir                = QString("%1/loras").arg(QAiUtils::modelsDir);
    embeddingsDir           = QString("%1/embeddings").arg(QAiUtils::modelsDir);
    controlNetDir           = QString("%1/controlNet").arg(QAiUtils::modelsDir);
    upscaleModelDir         = QString("%1/upscale").arg(QAiUtils::modelsDir);
    vaeDir                  = QString("%1/vae").arg(QAiUtils::modelsDir);
    audioVaeDir             = QString("%1/audioVae").arg(QAiUtils::modelsDir);

    packagesDir             = QStandardPaths::writableLocation(QStandardPaths::CacheLocation).append("/" + appName + "/packages");

    pluginsDir              = QString("%1/plugins").arg(QAiUtils::appRuntimeDir);
    extraQmlDir             = QString("%1/qml").arg(QAiUtils::appRuntimeDir);


    userDir                 = QString("%1/users").arg(baseDir);
    userJson                = QString("%1/users.json").arg(userDir);


}

QString QAiUtils::appName               = "qtai";
QString QAiUtils::baseDir               = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).append("/" + appName);

QString QAiUtils::configsDir            = QString("%1/configs").arg(baseDir);
QString QAiUtils::jsonConfigsDir        = QString("%1/json").arg(configsDir);
QString QAiUtils::yamlConfigsDir        = QString("%1/yaml").arg(configsDir);

QString QAiUtils::appRuntimeDir         = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).append(  "/" + appName);
QString QAiUtils::outDir                = QString("%1/out").arg(appRuntimeDir);
QString QAiUtils::logsDir               = QString("%1/logs").arg(appRuntimeDir);

QString QAiUtils::modelsDir             = QStandardPaths::writableLocation(QStandardPaths::CacheLocation).append(    "/" + appName + "/models");
QString QAiUtils::checkpointsDir        = QString("%1/checkpoints").arg(modelsDir);
QString QAiUtils::diffusionDir          = QString("%1/unet").arg(modelsDir);
QString QAiUtils::textEncoderDir        = QString("%1/textEncoders").arg(modelsDir);
QString QAiUtils::lorasDir              = QString("%1/loras").arg(modelsDir);
QString QAiUtils::embeddingsDir         = QString("%1/embeddings").arg(modelsDir);
QString QAiUtils::controlNetDir         = QString("%1/controlNet").arg(modelsDir);
QString QAiUtils::upscaleModelDir       = QString("%1/upscale").arg(modelsDir);
QString QAiUtils::vaeDir                = QString("%1/vae").arg(modelsDir);
QString QAiUtils::audioVaeDir           = QString("%1/audioVae").arg(modelsDir);

QString QAiUtils::packagesDir           = QStandardPaths::writableLocation(QStandardPaths::CacheLocation).append("/" + appName + "/packages");

QString QAiUtils::pluginsDir            = QString("%1/plugins").arg(appRuntimeDir);
QString QAiUtils::extraQmlDir           = QString("%1/qml").arg(appRuntimeDir);

QString QAiUtils::userDir               = QString("%1/users").arg(baseDir);
QString QAiUtils::userJson              = QString("%1/users.json").arg(userDir);

