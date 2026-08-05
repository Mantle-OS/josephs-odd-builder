#pragma once
#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <pointer-macros.h>

#include "qsdbaseparam.h"
#include "qsdimage.h"
#include "qmlsd_export.h"

// NOT TESTED YET
class QMLSD_EXPORT QSdPmParams : public QSdBaseParam
{
    Q_OBJECT
    QP_PTR_RO(QSdImage, idImages                ) // The base identity image used by PhotoMaker for character consistency.
    QP_RW(int,          idImagesCount,  0       ) // The number of identity images provided (wrapper currently targets 1).
    QP_RW(QString,      idEmbedPath,    ""      ) // Local file path to the PhotoMaker projection/embedding model.
    QP_RW(float,        styleStrength,  20.f    ) // The scaling weight of the PhotoMaker identity injection.
    QML_ELEMENT
public:
    explicit QSdPmParams(QObject *parent = nullptr):
        QSdBaseParam{parent},
        m_idImages{new QSdImage{}}
    {
        resetPmParams();
    }
    ~QSdPmParams()
    {
        if(m_idImages)
            delete m_idImages;
        m_idImages = nullptr;
    }

    sd_pm_params_t pmParams()
    {
        sd_pm_params_t ret{};
        if (!m_idImages->isNull()) {
            m_proxyIdImage = m_idImages->img();
            ret.id_images = &m_proxyIdImage;
        } else {
            ret.id_images = nullptr;
        }

        ret.id_images_count = m_idImagesCount;

        if(!m_idEmbedPath.isEmpty()){
            tmp_idEmbedPath = m_idEmbedPath.toLocal8Bit();
            ret.id_embed_path = tmp_idEmbedPath.constData();
        }else{
            ret.id_embed_path = nullptr;
        }

        ret.style_strength = m_styleStrength;

        m_pmParams = ret;
        return ret;
    }

    void setPmParams(sd_pm_params_t other)
    {
        if(other.id_images)
            m_idImages->setImg(*other.id_images);
        else
            m_idImages->resetImg();

        set_idImagesCount(other.id_images_count);
        set_idEmbedPath(other.id_embed_path ? QString::fromLatin1(other.id_embed_path) : QString{});
        set_styleStrength(other.style_strength);

        m_pmParams = other;
    }

    void resetPmParams()
    {
        m_pmParams = { nullptr, 0,  nullptr, 20.f };
        setPmParams(m_pmParams);
    }

private:
    QByteArray tmp_idEmbedPath;
    sd_pm_params_t m_pmParams = { nullptr, 0,  nullptr, 20.f };
    sd_image_t m_proxyIdImage{};
};
