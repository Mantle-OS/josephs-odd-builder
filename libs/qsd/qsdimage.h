#pragma once

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

#include "qmlsd_export.h"
class QMLSD_EXPORT QSdImage : public QQuickItem
{
    Q_OBJECT
    QP_RW(quint32,      channel,            0                                       ) // The number of color channels in the pixel data (e.g., 3 for RGB, 4 for RGBA).
    QP_RW(QString,      sourcePath,         ""                                      ) // The local file path if this image was loaded from or saved directly to disk.
    QP_RO(QString,      lastErrorString,    ""                                      ) // Not serlized on purpose
    Q_PROPERTY(quint8   *data               READ data   NOTIFY dataChanged  FINAL   )  // need this for c++ land only
    QML_ELEMENT

public:
    explicit QSdImage(QQuickItem *parent = nullptr);

    ~QSdImage() = default;

    bool isNull();

    sd_image_t img();
    void setImg(sd_image_t other);
    void resetImg();

    quint8 *data() const;

    void setData(quint8 *newData);
    static QImage::Format formatFromChannel(QSdEnums::QSdImageChannel chan);

    Q_INVOKABLE bool loadFromFile(const QString &filePath);
    Q_INVOKABLE bool saveToFile(const QString &filePath);

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &jsonObject);
    YAML::Node toYaml() const;
    void fromYaml(const YAML::Node &yamlNode);


protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

Q_SIGNALS:
    void dataChanged();
    void formatChanged();

private:
    sd_image_t  m_img{0, 0 ,0, nullptr};
    quint8      *m_data = nullptr;
    QImage      m_nativeImage;
    bool        m_imageChanged = false;
};
