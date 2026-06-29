#ifndef QSDIMAGE_H
#define QSDIMAGE_H

#include <QObject>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>

#include <QQmlEngine>
#include <QSGSimpleTextureNode>
#include <QSGTexture>

#include <QQuickItem>

#include <yaml-cpp/node/node.h>

#include <stable-diffusion.h>

#include <property-macros.h>

#include "qsdenums.h"

class QSdImage : public QQuickItem
{
    Q_OBJECT
    // width and height come from the QQuickItem
    QP_RW(quint32, channel, 0)
    QP_RW(QString, sourcePath, "")
    QP_RO(QString, lastErrorString, "")
    Q_PROPERTY(quint8 *data READ data NOTIFY dataChanged FINAL)

    QML_ELEMENT

public:
    explicit QSdImage(QQuickItem *parent = nullptr) :
        QQuickItem{parent}
    {
        setFlag(ItemHasContents, true);
        m_img = {0, 0, 0, nullptr};
    }

    ~QSdImage() = default;


    bool isNull()
    {
        return m_nativeImage.isNull();
    }


    sd_image_t img()
    {
        if (m_nativeImage.isNull()) {
            return {0, 0, 0, nullptr};
        }

        m_img.width = static_cast<uint32_t>(m_nativeImage.width());
        m_img.height = static_cast<uint32_t>(m_nativeImage.height());
        m_img.channel = static_cast<uint32_t>((m_nativeImage.format() == QImage::Format_RGBA8888) ? 4 : 3);
        m_img.data = const_cast<uint8_t*>(m_nativeImage.bits());

        return m_img;
    }

    void setImg(sd_image_t other)
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

        // Resolves your FIXME block cleanly via the enum wrapper utility mapping
        QImage::Format fmt = formatFromChannel(static_cast<QSdEnums::QSdImageChannel>(other.channel));
        if (fmt == QImage::Format_Invalid) {
            fmt = QImage::Format_RGB888; // Fail-safe fallback
        }

        QImage wrapper(other.data, other.width, other.height, other.width * other.channel, fmt);
        m_nativeImage = wrapper.copy(); // Deep copy isolation

        m_img = other;
        m_img.data = const_cast<uint8_t*>(m_nativeImage.bits());
        m_imageChanged = true;

        Q_EMIT dataChanged();
        update(); // Request Scene Graph content refresh pass
    }

    void resetImg()
    {
        sd_image_t blank{0, 0, 0, nullptr};
        setImg(blank);
        set_sourcePath("");
    }

    quint8* data() const { return m_nativeImage.isNull() ? nullptr : const_cast<uint8_t*>(m_nativeImage.bits()); }

    void setData(quint8 *newData)
    {
        if (m_data == newData) return;
        m_data = newData;
        m_img.data = newData;
        Q_EMIT dataChanged();
    }

    static QImage::Format formatFromChannel(QSdEnums::QSdImageChannel chan)
    {
        switch (chan) {
        case QSdEnums::QSdGrayscale8:   return QImage::Format_Grayscale8;
        case QSdEnums::RGB888:          return QImage::Format_RGB888;
        case QSdEnums::RGBA8888:        return QImage::Format_RGBA8888;
        default:                        return QImage::Format_Invalid;
        }
    }

    Q_INVOKABLE bool loadFromFile(const QString &filePath)
    {
        QImage loaded;
        if (!loaded.load(filePath)) return false;

        // Force multi-platform target channel alignment
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

    Q_INVOKABLE bool saveToFile(const QString &filePath)
    {
        if (m_nativeImage.isNull()) {
            set_lastErrorString("Cannot save: Internal image buffer is empty.");
            return false;
        }

        QFileInfo fileInfo(filePath);
        QDir dir = fileInfo.dir();
        QString baseName = fileInfo.baseName(); // Filename without extension or path
        QString extension = fileInfo.suffix(); // e.g., "png" or "jpg"

        // Default fallback if suffix extension is entirely omitted
        if (extension.isEmpty()) {
            extension = "png";
        }

        QString finalFilePath = filePath;

        // 1. Check if the exact requested filename already exists
        if (QFile::exists(filePath)) {
            int highestIndex = 0;

            // Match files with the pattern: baseName_XXX.extension
            // Using a wildcard filter to scan the directory
            QString filter = QString("%1_*.%2").arg(baseName, extension);
            QStringList existingFiles = dir.entryList(QStringList(filter), QDir::Files);

            // Regular expression to parse out the suffix digits
            // e.g., matches "_003" capturing "003"
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

            // Increment to get the next sequential number in the series
            int nextIndex = highestIndex + 1;

            // Pad with leading zeros out to 3 digits (e.g., _001, _012, _104)
            QString suffix = QString("_%1").arg(nextIndex, 3, 10, QChar('0'));
            finalFilePath = dir.absoluteFilePath(QString("%1%2.%3").arg(baseName, suffix, extension));
        }

        // 2. Perform the actual hardware write operation via QImage
        // Qt automatically determines the compression algorithm from the file extension
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

    QJsonObject toJson() const
    {
        QJsonObject ret;
        ret["width"] = width();
        ret["height"] = height();
        ret["channel"] = static_cast<int>(m_channel);
        ret["sourcePath"] = m_sourcePath;
        return ret;
    }

    void fromJson(const QJsonObject &jsonObject)
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

    YAML::Node toYaml() const
    {
        YAML::Node ret;
        ret["width"] = width();
        ret["height"] = height();
        ret["channel"] = static_cast<int>(m_channel);
        ret["sourcePath"] = m_sourcePath.toStdString();
        return ret;
    }

    void fromYaml(const YAML::Node &yamlNode)
    {
        if (yamlNode["sourcePath"]) {
            std::string path = yamlNode["sourcePath"].as<std::string>();
            if (!path.empty()) {
                loadFromFile(QString::fromStdString(path));
                return;
            }
        }
        if (yamlNode["width"])   setWidth(yamlNode["width"].as<double>());
        if (yamlNode["height"])  setHeight(yamlNode["height"].as<double>());
        if (yamlNode["channel"]) set_channel(yamlNode["channel"].as<int>());
    }


protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override
    {
        Q_UNUSED(data)

        // If the geometry width/height is zero in QML, skip rendering completely
        if (width() <= 0 || height() <= 0 || m_nativeImage.isNull()) {
            delete oldNode;
            return nullptr;
        }

        QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);
        if (!node) {
            node = new QSGSimpleTextureNode();
        }

        // Only upload the texture to the GPU hardware layers if the pixels actually changed
        if (m_imageChanged) {
            // Automatically instantiates the underlying texture resource depending on backend (Vulkan/OpenGL/DirectX)
            QSGTexture *texture = window()->createTextureFromImage(m_nativeImage);

            // Ownership transfer: node deletes the texture wrapper automatically when done
            node->setOwnsTexture(true);
            node->setTexture(texture);
            m_imageChanged = false;
        }

        // Map the texture dimensions to your bounding box area inside your QML layout
        node->setRect(0, 0, width(), height());
        node->setFiltering(QSGTexture::Linear); // Clean anti-aliasing interpolation scaling

        return node;
    }


Q_SIGNALS:
    void dataChanged();
    void formatChanged();
    // void imageChanged();

private:
    sd_image_t  m_img{0, 0 ,0, nullptr};
    quint8      *m_data = nullptr;
    QImage      m_nativeImage;
    bool        m_imageChanged = false;
};

#endif  // QSDIMAGE_H
