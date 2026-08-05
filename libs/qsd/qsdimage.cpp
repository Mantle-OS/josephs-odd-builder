#include "qsdimage.h"
#include <yaml-cpp/yaml.h>
QSdImage::QSdImage(QQuickItem *parent) :
    QQuickItem{parent}
{
    setFlag(ItemHasContents, true);
    m_img = {0, 0, 0, nullptr};
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

bool QSdImage::isNull()
{
    return m_nativeImage.isNull();
}

sd_image_t QSdImage::img()
{
    if (m_nativeImage.isNull())
        return {0, 0, 0, nullptr};

    m_img.width = static_cast<uint32_t>(m_nativeImage.width());
    m_img.height = static_cast<uint32_t>(m_nativeImage.height());
    m_img.channel = static_cast<uint32_t>((m_nativeImage.format() == QImage::Format_RGBA8888) ? 4 : 3);
    m_img.data = const_cast<uint8_t*>(m_nativeImage.bits());

    return m_img;
}

void QSdImage::setImg(sd_image_t other)
{
    set_channel(other.channel);
    setWidth(other.width);
    setHeight(other.height);

    if (other.data == nullptr || other.width == 0 || other.height == 0) {
        m_nativeImage = QImage();
        m_img = {0, 0, 0, nullptr};
        m_imageChanged = true;
        update();
        return;
    }

    QImage::Format fmt = formatFromChannel(static_cast<QSdEnums::QSdImageChannel>(other.channel));
    if (fmt == QImage::Format_Invalid)
        fmt = QImage::Format_RGB888; //  fallback

    QImage wrapper(other.data, other.width, other.height, other.width * other.channel, fmt);
    m_nativeImage = wrapper.copy(); // Deep copy :(  whatever its still fast

    m_img = other;
    m_img.data = const_cast<uint8_t*>(m_nativeImage.bits());
    m_imageChanged = true;

    Q_EMIT dataChanged();
    update();
}

void QSdImage::resetImg()
{
    sd_image_t blank{0, 0, 0, nullptr};
    setImg(blank);
    set_sourcePath("");
}

quint8 *QSdImage::data() const
{
    return m_nativeImage.isNull() ? nullptr : const_cast<uint8_t*>(m_nativeImage.bits());
}

void QSdImage::setData(quint8 *newData)
{
    if (m_data == newData) return;
    m_data = newData;
    m_img.data = newData;
    Q_EMIT dataChanged();
}

QImage::Format QSdImage::formatFromChannel(QSdEnums::QSdImageChannel chan)
{
    switch (chan) {
    case QSdEnums::QSdGrayscale8:   return QImage::Format_Grayscale8;
    case QSdEnums::RGB888:          return QImage::Format_RGB888;
    case QSdEnums::RGBA8888:        return QImage::Format_RGBA8888;
    default:                        return QImage::Format_Invalid;
    }
}

bool QSdImage::loadFromFile(const QString &filePath)
{
    QImage loaded;
    if (!loaded.load(filePath))
        return false;

    // I will need to add the others later
    m_nativeImage = loaded.convertToFormat(loaded.hasAlphaChannel() ?
                                               QImage::Format_RGBA8888 : QImage::Format_RGB888);

    // Match base dimension sizes natively
    setWidth(m_nativeImage.width());
    setHeight(m_nativeImage.height());
    set_channel(m_nativeImage.hasAlphaChannel() ? 4 : 3);
    set_sourcePath(filePath);

    m_imageChanged = true;
    Q_EMIT dataChanged();
    update();
    return true;
}

bool QSdImage::saveToFile(const QString &filePath)
{
    if (m_nativeImage.isNull()) {
        set_lastErrorString("Cannot save: Internal image buffer is empty.");
        return false;
    }

    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.dir();
    QString baseName = fileInfo.baseName();
    QString extension = fileInfo.suffix();
    if (extension.isEmpty()) {
        extension = "png";
    }

    QString finalFilePath = filePath;

    if (QFile::exists(filePath)) {
        int highestIndex = 0;

        QString filter = QString("%1_*.%2").arg(baseName, extension);
        QStringList existingFiles = dir.entryList(QStringList(filter), QDir::Files);


        // regex for example "_003" capturing "003"
        static const QRegularExpression regex(QString("^%1_(\\d+)\\.%2$").arg(QRegularExpression::escape(baseName), QRegularExpression::escape(extension)));

        for (const QString &filename : existingFiles) {
            QRegularExpressionMatch match = regex.match(filename);
            if (match.hasMatch()) {
                int currentIndex = match.captured(1).toInt();
                if (currentIndex > highestIndex) {
                    highestIndex = currentIndex;
                }
            }
        }

        int nextIndex = highestIndex + 1;

        // (e.g., _001, _012, _104)
        QString suffix = QString("_%1").arg(nextIndex, 3, 10, QChar('0'));
        finalFilePath = dir.absoluteFilePath(QString("%1%2.%3").arg(baseName, suffix, extension));
    }

    // Perform the actual hardware write operation via QImage -> Qt automatically determines the compression algorithm from the file extension
    bool success = m_nativeImage.save(finalFilePath);

    if (!success) {
        set_lastErrorString(QString("Failed to write image data to file path: %1").arg(finalFilePath));
        qWarning() << "QSdImage: Save failed for target path:" << finalFilePath;
    } else {
        qDebug() << "QSdImage: Image successfully saved to:" << finalFilePath;
        // Optional: Update source path to point to the newly minted local file
        set_sourcePath(finalFilePath);
    }

    return success;
}

QJsonObject QSdImage::toJson() const
{
    QJsonObject ret;
    ret["width"] = m_nativeImage.isNull() ? 0 : m_nativeImage.width();
    ret["height"] = m_nativeImage.isNull() ? 0 : m_nativeImage.height();
    ret["channel"] = static_cast<int>(m_channel);
    ret["sourcePath"] = m_sourcePath;
    return ret;
}

void QSdImage::fromJson(const QJsonObject &jsonObject)
{
    if (jsonObject.contains("sourcePath")) {
        QString path = jsonObject["sourcePath"].toString();
        if (!path.isEmpty()) {
            loadFromFile(path);
            return;
        }
    }
    if (jsonObject.contains("width"))  setWidth(jsonObject["width"].toInt());
    if (jsonObject.contains("height")) setHeight(jsonObject["height"].toInt());
    if (jsonObject.contains("channel")) set_channel(jsonObject["channel"].toInt());
}

YAML::Node QSdImage::toYaml() const
{
    YAML::Node ret;
    ret["width"] = width();
    ret["height"] = height();
    ret["channel"] = static_cast<int>(m_channel);
    ret["sourcePath"] = m_sourcePath.toStdString();
    return ret;
}

void QSdImage::fromYaml(const YAML::Node &yamlNode)
{
    if (yamlNode["sourcePath"]) {
        std::string path = yamlNode["sourcePath"].as<std::string>();
        if (!path.empty()) {
            loadFromFile(QString::fromStdString(path));
            return;
        }
    }
    if (yamlNode["width"])
        setWidth(yamlNode["width"].as<double>());
    if (yamlNode["height"])
        setHeight(yamlNode["height"].as<double>());
    if (yamlNode["channel"])
        set_channel(yamlNode["channel"].as<int>());
}

QSGNode *QSdImage::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data)

    if (width() <= 0 || height() <= 0 || m_nativeImage.isNull()) {
        delete oldNode;
        return nullptr;
    }

    QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);
    if (!node)
        node = new QSGSimpleTextureNode();

    // Only upload the texture to the GPU hardware layers if the pixels actually changed
    if (m_imageChanged) {
        QSGTexture *texture = window()->createTextureFromImage(m_nativeImage);
        node->setTexture(texture);
        node->setOwnsTexture(true);
        m_imageChanged = false;
    }

    node->setRect(0, 0, width(), height());
    node->setFiltering(QSGTexture::Linear);

    return node;
}
