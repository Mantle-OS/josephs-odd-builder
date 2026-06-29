#ifndef QSDPMPARAMS_H
#define QSDPMPARAMS_H

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <pointer-macros.h>

#include "qsdbaseparam.h"
#include "qsdimage.h"

class QSdPmParams : public QSdBaseParam
{
    Q_OBJECT
    QP_PTR_RO(QSdImage, idImages)

    QP_RW(int,          idImagesCount,  0       )
    QP_RW(QString,      idEmbedPath,    ""      )
    QP_RW(float,        styleStrength,  20.f    )
    QML_ELEMENT
public:
    explicit QSdPmParams(QObject *parent = nullptr):
        m_idImages{new QSdImage{}},
        QSdBaseParam{parent}
    {
        resetPmParams();
    }
    ~QSdPmParams()
    {
        resetPmParams();
        if(m_idImages){
            delete m_idImages;
            m_idImages = nullptr;
        }
    }

    sd_pm_params_t pmParams()
    {
        sd_pm_params_t ret{};

        // ret.id_images = m_idImages->img();
        // if(m_idImages->img())
        // else
        //     ret.id_images = nullptr;

        ret.id_images_count = m_idImagesCount;

        if(!m_idEmbedPath.isEmpty()){
            tmp_idEmbedPath = m_idEmbedPath.toLocal8Bit();
            ret.id_embed_path = tmp_idEmbedPath.constData();
        }else{
            ret.id_embed_path = nullptr;
        }

        ret.style_strength = m_styleStrength;
        return ret;
    }
    void setPmParams(sd_pm_params_t other)
    {
        if(other.id_images)
            m_idImages->setImg(*other.id_images);
        set_idImagesCount(other.id_images_count);
        set_idEmbedPath(QString::fromLatin1(other.id_embed_path));
        set_styleStrength(other.style_strength);
    }
    void resetPmParams()
    {
        m_pmParams = { nullptr, 0,  nullptr, 20.f };
        setPmParams(m_pmParams);
    }

private:
    QByteArray tmp_idEmbedPath;
    sd_pm_params_t m_pmParams = { nullptr, 0,  nullptr, 20.f };
};

#endif // QSDPMPARAMS_H
