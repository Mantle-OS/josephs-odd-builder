#ifndef QSDIMGGENPARAMS_H
#define QSDIMGGENPARAMS_H

#include <QObject>
#include <QQmlEngine>

#include <stable-diffusion.h>

#include <objectmodel.h>
#include <pointer-macros.h>

#include "qsdcacheparams.h"
#include "qsdhiresparams.h"
#include "qsdlora.h"
#include "qsdimage.h"
#include "qsdsampleparams.h"
#include "qsdpmparams.h"
#include "qsdtilingparams.h"

class QSdImgGenParams : public QSdBaseParam
{
    Q_OBJECT
    // Q_PROPERTY(quint32 loraCount)
    QP_PTR_RO(ObjectListModel<QSdLora>, loras                           )
    QP_RW(QString,                      prompt ,                ""      )
    QP_RW(QString,                      negativePrompt,         ""      )
    QP_RW(int,                          clipSkip,               -1      )
    QP_PTR_RW(QSdImage,                 initImage                       ) // here
    QP_PTR_RO(QSdImage,                 refImages                       ) // here
    QP_RW(int,                          refImagesCount,         0       )
    QP_RW(bool,                         autoResizeRefImage,     false   )
    QP_RW(bool,                         increaseRefIndex,       false   )
    QP_PTR_RW(QSdImage,                 maskImage                       ) // here
    QP_RW(int,                          imgWidth,               512     )
    QP_RW(int,                          imgHeight,              512     )
    QP_PTR_RW(QSdSampleParams,          sampleParams                    )
    QP_RW(float,                        strength,               0.75f   )
    QP_RW(qint64,                       seed,                   -1      )
    QP_RW(int,                          batchCount,             1       )
    QP_PTR_RW(QSdImage,                 controlImage                    ) // here
    QP_RW(float,                        controlStrength,        0.9f    )
    QP_PTR_RO(QSdPmParams,              pmParams                        )
    QP_PTR_RO(QSdTilingParams,          vaeTilingParams                 )
    QP_PTR_RO(QSdCacheParams,           cache                           )
    QP_PTR_RO(QSdHiResParams,           hires                           )
    QML_ELEMENT
    QML_UNCREATABLE("Please Use QSD.GenerationParams")
public:
    explicit QSdImgGenParams(QObject *parent = nullptr);
    ~QSdImgGenParams();

    Q_INVOKABLE QString debugString();

    sd_img_gen_params_t imgGenParms()
    {
        sd_img_gen_params_t ret{};
        m_loraVec.clear();
        //+ high_noise_lora_map.size()
        m_loraVec.reserve(m_loras->size() );
        for (QSdLora *i : m_loras->toList()) {
            // sd_lora_t lo = i->lora();
            m_loraVec.push_back(i->lora());
            // FIXME LATER
        }

        ret.loras      = m_loraVec.empty() ? nullptr : m_loraVec.data();
        ret.lora_count = m_loras->size();

        if(!m_prompt.isEmpty()){
            tmp_prompt = m_prompt.toLocal8Bit();
            ret.prompt                    = tmp_prompt.constData();
        } else {
            ret.prompt                    = nullptr;
        }
        if(!m_negativePrompt.isEmpty()){
            tmp_negativePrompt = m_negativePrompt.toLocal8Bit();
            m_imgGenParms.negative_prompt   = tmp_negativePrompt.constData();
        } else {
            ret.negative_prompt             = nullptr;
        }
        ret.clip_skip                       = m_clipSkip;
        ret.init_image                      = m_initImage->img();
        // if()
        // m_imgGenParms.ref_images                = nullptr; // FIXME
        ret.ref_images_count                = m_refImagesCount;
        ret.auto_resize_ref_image           = m_autoResizeRefImage;
        ret.increase_ref_index              = m_increaseRefIndex;
        ret.mask_image                      = m_maskImage->img();
        ret.width                           = m_imgWidth;
        ret.height                          = m_imgHeight;
        ret.sample_params                   = m_sampleParams->sampleParams();
        ret.strength                        = m_strength;
        ret.seed                            = m_seed;
        ret.batch_count                     = m_batchCount;
        ret.control_image                   = m_controlImage->img();
        ret.control_strength                = m_controlStrength;
        ret.pm_params                       = m_pmParams->pmParams();
        ret.vae_tiling_params               = m_vaeTilingParams->tilingParams();
        ret.cache                           = m_cache->cacheParams();
        ret.hires                           = m_hires->hiresParams();

        return ret;
    }
    void setImgGenParms(sd_img_gen_params_t other)
    {

        if(other.loras){
            for(uint32_t i = 0; i < other.lora_count; i++){
                QString lora_fileName = QString::fromLatin1(other.loras[i].path);
                QSdLora *nLora = m_loras->getByUid(lora_fileName);
                if(nLora == Q_NULLPTR){
                    nLora = new QSdLora;
                    nLora->setLora(other.loras[i]);
                } else {
                    nLora->setLora(other.loras[i]);
                }
            }
        }

        if(other.prompt){
            set_prompt(QString::fromLatin1(other.prompt));
        }else{
            set_prompt("");
        }
        if(other.negative_prompt){
            set_negativePrompt(QString::fromLatin1(other.negative_prompt));
        }else{
            set_negativePrompt("");
        }

        set_clipSkip(other.clip_skip);

        m_initImage->setImg(other.init_image);
        if(m_refImages){
            m_refImages->setImg(*other.ref_images);
        }
        set_refImagesCount(other.ref_images_count);
        set_autoResizeRefImage(other.auto_resize_ref_image);
        set_increaseRefIndex(other.increase_ref_index);

        m_maskImage->setImg(other.mask_image);

        set_imgHeight(other.height);
        set_imgWidth(other.width);

        m_sampleParams->setSampleParams(other.sample_params);
        set_strength(other.strength);
        set_seed(other.seed);
        set_batchCount(other.batch_count);

        m_controlImage->setImg(other.control_image);
        set_controlStrength(other.control_strength);

        m_pmParams->setPmParams(other.pm_params);
        m_vaeTilingParams->setTilingParams(other.vae_tiling_params);
        m_cache->setCacheParams(other.cache);
        m_hires->setHiresParams(other.hires);
    }

    void resetImgGenParms()
    {
        if(!m_loras->isEmpty())
            m_loras->clear();
        m_imgGenParms.loras                     = nullptr;
        m_imgGenParms.lora_count                = 0;

        m_imgGenParms.prompt                    = nullptr;
        m_imgGenParms.negative_prompt           = nullptr;
        m_imgGenParms.clip_skip                 = -1;

        m_imgGenParms.init_image                = m_initImage->img();

        m_imgGenParms.ref_images                = nullptr;
        m_imgGenParms.ref_images_count          = 0;
        m_imgGenParms.auto_resize_ref_image     = true;
        m_imgGenParms.increase_ref_index        = false;

        m_imgGenParms.mask_image                = m_maskImage->img();
        m_imgGenParms.width                     = 512;
        m_imgGenParms.height                    = 512;

        m_sampleParams->resetSampleParams();
        m_imgGenParms.sample_params = m_sampleParams->sampleParams();

        m_imgGenParms.strength                  = 0.75f;
        m_imgGenParms.seed                      = -1;
        m_imgGenParms.batch_count               = 1;

        m_imgGenParms.control_image = m_controlImage->img();
        m_imgGenParms.control_strength          = 0.9f;

        m_pmParams->resetPmParams();
        m_imgGenParms.pm_params                 = m_pmParams->pmParams();

        m_vaeTilingParams->resetTilingParams();
        m_imgGenParms.vae_tiling_params         = m_vaeTilingParams->tilingParams();

        m_cache->resetCacheParams();
        m_imgGenParms.cache                     = m_cache->cacheParams();

        m_hires->resetHiresParams();
        m_imgGenParms.hires                     = m_hires->hiresParams();

        setImgGenParms(m_imgGenParms);
    }

private:
    sd_img_gen_params_t m_imgGenParms{};
    std::vector<sd_lora_t> m_loraVec;

    QByteArray tmp_prompt;
    QByteArray tmp_negativePrompt;
};

#endif // QSDIMGGENPARAMS_H
