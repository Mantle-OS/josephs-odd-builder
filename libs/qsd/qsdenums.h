#ifndef QSDENUMS_H
#define QSDENUMS_H

#include "core/ggml_extend_backend.h"
#include <QObject>
#include <QMap>
#include <QtQmlIntegration>
#include <stable-diffusion.h>

// the following function to mimic stable-diffusion.h API calls
// enums has the following rules
// public
// getSd_XXXX_Type: static method returns the C enum from stable-diffusion.h from our qml exposed enum
// getQSd_XXXX_Type: static method returns the mapping to our enums that are in ther metaobject system now (gui land)
// XXXX_TypeName(): returns a string neme from the enum
// XXXX_TypeFromString(): returns the enum from the passed in the string. returns QSdCount if nothing is in the stack
// private:
// map<ENUM, string_name>
// readonly(in qml) stringlist for gui land
// fillmap that fills the datasets on startup
// clear map that has the calls to clear the map and stringlist



class QSdEnums : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList weightTypesList READ weightTypesList NOTIFY weightTypesListChanged FINAL)
    Q_PROPERTY(QStringList rngTypesList READ rngTypesList NOTIFY rngTypesListChanged FINAL)
    Q_PROPERTY(QStringList sampleTypesList READ sampleTypesList NOTIFY sampleTypesListChanged FINAL)
    Q_PROPERTY(QStringList schedulerTypesList READ schedulerTypesList NOTIFY schedulerTypesListChanged FINAL)
    Q_PROPERTY(QStringList predictionTypesList READ predictionTypesList NOTIFY predictionTypesListChanged FINAL)
    Q_PROPERTY(QStringList previewTypesList READ previewTypesList NOTIFY previewTypesListChanged FINAL)
    Q_PROPERTY(QStringList loraApplyModeTypesList READ loraApplyModeTypesList NOTIFY loraApplyModeTypesListChanged FINAL)
    Q_PROPERTY(QStringList vaeFormatTypesList READ vaeFormatTypesList NOTIFY vaeFormatTypesListChanged FINAL)
    Q_PROPERTY(QStringList logLevelTypesList READ logLevelTypesList NOTIFY logLevelTypesListChanged FINAL)
    Q_PROPERTY(QStringList cacheModeTypesList READ cacheModeTypesList NOTIFY cacheModeTypesListChanged FINAL)
    Q_PROPERTY(QStringList hiResUpscalerTypesList READ hiResUpscalerTypesList NOTIFY hiResUpscalerTypesListChanged FINAL)

    QML_ELEMENT
    QML_SINGLETON
public:
    explicit QSdEnums(QObject *parent = nullptr);
    ~QSdEnums();

    enum QSdWeightTypes {
        QSdF32  = 0,
        QSdF16  = 1,
        QSdQ4_0 = 2,
        QSdQ4_1 = 3,
        // SD_TYPE_Q4_2 = 4, support has been removed
        // SD_TYPE_Q4_3 = 5, support has been removed
        QSdQ5_0    = 6,
        QSdQ5_1    = 7,
        QSdQ8_0    = 8,
        QSdQ8_1    = 9,
        QSdQ2_K    = 10,
        QSdQ3_K    = 11,
        QSdQ4_K    = 12,
        QSdQ5_K    = 13,
        QSdQ6_K    = 14,
        QSdQ8_K    = 15,
        QSdIQ2_XXS = 16,
        QSdIQ2_XS  = 17,
        QSdIQ3_XXS = 18,
        QSdIQ1_S   = 19,
        QSdIQ4_NL  = 20,
        QSdIQ3_S   = 21,
        QSdIQ2_S   = 22,
        QSdIQ4_XS  = 23,
        QSdI8      = 24,
        QSdI16     = 25,
        QSdI32     = 26,
        QSdI64     = 27,
        QSdF64     = 28,
        QSdIQ1_M   = 29,
        QSdBF16    = 30,
        // SD_TYPE_Q4_0_4_4 = 31, support has been removed from gguf files
        // SD_TYPE_Q4_0_4_8 = 32,
        // SD_TYPE_Q4_0_8_8 = 33,
        QSdTQ1_0 = 34,
        QSdTQ2_0 = 35,
        // SD_TYPE_IQ4_NL_4_4 = 36,
        // SD_TYPE_IQ4_NL_4_8 = 37,
        // SD_TYPE_IQ4_NL_8_8 = 38,
        QSdMXFP4 = 39,  // MXFP4 (1 block)
        QSdNVFP4 = 40,  // NVFP4 (4 blocks, E4M3 scale)
        QSdQ1_0  = 41,
        QSdCount = 42,
    };
    Q_ENUM(QSdWeightTypes)
    QStringList weightTypesList() const;
    static sd_type_t sdWeightType(const QSdWeightTypes &sdType);
    static QSdWeightTypes qsdWeightType(const sd_type_t &type);
    Q_INVOKABLE QString weightTypeName(QSdWeightTypes type);
    Q_INVOKABLE QSdWeightTypes weightTypeFromString(const QString &type);
    Q_INVOKABLE int weightTypeIndexAt(const QString &in);
    Q_INVOKABLE int weightTypeIndexAt(const QSdWeightTypes &sdType);

    //////////
    enum QSdRngTypes {
        QSdStdDefaultRng,
        QSdCudaRNG,
        QSdCpuRNG,
        QSdRngTypeCount
    };
    Q_ENUM(QSdRngTypes)
    QStringList rngTypesList() const;
    static rng_type_t sdRngType(const QSdRngTypes rngType);
    static QSdRngTypes qsdRngType(rng_type_t rngType);
    Q_INVOKABLE QString rngTypeName(QSdRngTypes type);
    Q_INVOKABLE QSdRngTypes rngTypeFromString(const QString &type);

    ////////////////////////////
    enum QSdSampleTypes {
        QSdEuler,
        QSdEulerA,
        QSdHeun,
        QSdDpm2,
        QSdDpmPP2SA,
        QSdDpmPP2M,
        QSdDpmPP2Mv2,
        QSdIpndm,
        QSdIpndmV,
        QSdLcmSampler,
        QSdDddmTrailing,
        QSdTcd,
        QSdResMultistep,
        QSdRes2S,
        QSdErSde,
        QSdEulerCfgPp,
        QSdEulerACfpPp,
        QSdEulerGe,
        QSdSampleCount
    };
    Q_ENUM(QSdSampleTypes)
    QStringList sampleTypesList() const;
    static sample_method_t sdSampleType(const QSdSampleTypes &type);
    static QSdSampleTypes qsdSampleType(sample_method_t type);
    Q_INVOKABLE QString sampleTypeName(QSdSampleTypes type);
    Q_INVOKABLE QSdSampleTypes sampleTypeFromString(const QString &type);


    ////////////////////////////////////////
    enum QSdSchedulerTypes {
        QSdDiscrete,
        QSdKarras,
        QSdExponential,
        QSdAys,
        QSdGits,
        QSdSgmUniform,
        QSdSimple,
        QSdSmoothstep,
        QSdKlOptimal,
        QSdLcmScheduler,
        QSdBong,
        QSdLtx2,
        QSdSchedulerCount // Scheduler
    };
    Q_ENUM(QSdSchedulerTypes)
    QStringList schedulerTypesList() const;
    static scheduler_t sdSchedulerType(const QSdSchedulerTypes &type);
    static QSdSchedulerTypes qsdSchedulerType(scheduler_t type);
    Q_INVOKABLE QString schedulerTypeName(QSdSchedulerTypes type);
    Q_INVOKABLE QSdSchedulerTypes schedulerTypeFromString(const QString &type);

    ////////////////////////////////////////
    enum QSdPredictionTypes {
        QSdEps,
        QSdV,
        QSdEdmV,
        QSdFlow,
        QSdFluxFlow,
        QSdFlux2Flow,
        QSdPredictionCount
    };
    Q_ENUM(QSdPredictionTypes)
    QStringList predictionTypesList() const;
    static prediction_t sdPredictionType(const QSdPredictionTypes &type);
    static QSdPredictionTypes qsdPredictionType(prediction_t type);
    Q_INVOKABLE QString schedulerTypeName(QSdPredictionTypes type);
    Q_INVOKABLE QSdPredictionTypes predictionTypeFromString(const QString &type);

    ////////////////////////////////////////
    enum QSdPreviewTypes{
        QSdPreviewNone,
        QSdPreviewProj,
        QSdPreviewTae,
        QSdPreviewVae,
        QSdPreviewCount
    };
    Q_ENUM(QSdPreviewTypes)
    QStringList previewTypesList() const;
    static preview_t sdPreviewType(const QSdPreviewTypes &type);
    static QSdPreviewTypes qsdPreviewType(preview_t type);
    Q_INVOKABLE QString previewTypeName(QSdPreviewTypes type);
    Q_INVOKABLE QSdPreviewTypes previewTypeFromString(const QString &type);

    ////////////////////////////////////////
    enum QSdLoraApplyModeTypes {
        QSdLoraAuto,
        QSdLoraImmediately,
        QSdLoraAtRuntime,
        QSdLoraCount,
    };
    Q_ENUM(QSdLoraApplyModeTypes)
    QStringList loraApplyModeTypesList() const;
    static lora_apply_mode_t sdLoraApplyModeType(const QSdLoraApplyModeTypes &type);
    static QSdLoraApplyModeTypes qsdLoraApplyModeType(lora_apply_mode_t type);
    Q_INVOKABLE QString loraApplyModeTypeName(QSdLoraApplyModeTypes type);
    Q_INVOKABLE QSdLoraApplyModeTypes loraApplyModeTypeFromString(const QString &type);

    ////////////////////////////////////////
    enum QSdVaeFormatTypes {
        QSdVaeFormatAuto = -1,
        QSdVaeFormatFlux,
        QSdVaeFormatSd3,
        QSdVaeFormatFlux2,
        QSdVaeFormatCount,
    };
    Q_ENUM(QSdVaeFormatTypes)
    QStringList vaeFormatTypesList() const;
    static sd_vae_format_t sdVaeFormatType(const QSdVaeFormatTypes &type);
    static QSdVaeFormatTypes qsdVaeFormatType(sd_vae_format_t type);
    Q_INVOKABLE QString vaeFormatTypeName(QSdVaeFormatTypes type);
    Q_INVOKABLE QSdVaeFormatTypes vaeFormatTypeFromString(const QString &type);


    ////////////////////////////////////////
    enum QSdLogLevelTypes {
        QSdLogDebug,
        QSdLogInfo,
        QSdLogWarn,
        QSdLogError
    };
    Q_ENUM(QSdLogLevelTypes)
    QStringList logLevelTypesList() const;
    static sd_log_level_t sdLogLevelType(const QSdLogLevelTypes &type);
    static QSdLogLevelTypes qsdLogLevelType(sd_log_level_t type);
    Q_INVOKABLE QString logLevelTypeName(QSdLogLevelTypes type);
    Q_INVOKABLE QSdLogLevelTypes logLevelTypeFromString(const QString &type);


    ////////////////////////////////////////
    enum QSdCacheModeTypes {
        QSdCacheDisabled = 0,
        QSdCacheEasycache,
        QSdCacheUcache,
        QSdCacheDbcache,
        QSdCacheTaylorseer,
        QSdCacheCacheDit,
        QSdCacheSpectrum,
    };
    Q_ENUM(QSdCacheModeTypes)
    QStringList cacheModeTypesList() const;
    static sd_cache_mode_t sdCacheModeType(const QSdCacheModeTypes &type);
    static QSdCacheModeTypes qsdCacheModeType(sd_cache_mode_t type);
    Q_INVOKABLE QString cacheModeTypeName(QSdCacheModeTypes type);
    Q_INVOKABLE QSdCacheModeTypes cacheModeTypeFromString(const QString &type);



    ////////////////////////////////////////
    enum QSdHiResUpscalerTypes {
        QSdHiResUpscalerNone,
        QSdHiResUpscalerLatent,
        QSdHiResUpscalerLatentNearest,
        QSdHiResUpscalerLatentNearestExact,
        QSdHiResUpscalerLatentAntialiased,
        QSdHiResUpscalerLatentBicubic,
        QSdHiResUpscalerLatentBicubicAntialiased,
        QSdHiResUpscalerLanczos,
        QSdHiResUpscalerNearest,
        QSdHiResUpscalerModel,
        QSdHiResUpscalerCount,
    };
    Q_ENUM(QSdHiResUpscalerTypes)
    QStringList hiResUpscalerTypesList() const;
    static sd_hires_upscaler_t sdHiResUpscalerType(const QSdHiResUpscalerTypes &type);
    static QSdHiResUpscalerTypes qsdHiResUpscalerType(sd_hires_upscaler_t type);
    Q_INVOKABLE QString hiResUpscalerTypeName(QSdHiResUpscalerTypes type);
    Q_INVOKABLE QSdHiResUpscalerTypes hiResUpscalerTypeFromString(const QString &type);






    // Later I will add this to make things a bit cleaner
    enum QSdImageChannel{
        QSdGrayscale8 = 1,
        RGB888 = 3,
        RGBA8888 = 4
    };
    Q_ENUM(QSdImageChannel)

    enum QSdBackendModule {
        Diffusion  = static_cast<int>(SDBackendModule::DIFFUSION),
        TextEncoder= static_cast<int>(SDBackendModule::TE),
        ClipVision = static_cast<int>(SDBackendModule::CLIP_VISION),
        Vae        = static_cast<int>(SDBackendModule::VAE),
        ControlNet = static_cast<int>(SDBackendModule::CONTROL_NET),
        PhotoMaker = static_cast<int>(SDBackendModule::PHOTOMAKER),
        Upscaler   = static_cast<int>(SDBackendModule::UPSCALER)
    };
    Q_ENUM(QSdBackendModule)



Q_SIGNALS:
    void weightTypesListChanged();
    void rngTypesListChanged();
    void sampleTypesListChanged();
    void schedulerTypesListChanged();
    void predictionTypesListChanged();
    void previewTypesListChanged();
    void loraApplyModeTypesListChanged();
    void vaeFormatTypesListChanged();
    void logLevelTypesListChanged();
    void cacheModeTypesListChanged();
    void hiResUpscalerTypesListChanged();

private:
    void fillAllTypes();
    void clearAllTypes();

    QMap<QSdWeightTypes, QString> m_weightTypesMap;
    QStringList m_weightTypesList;
    void fillWeightTypes();

    QMap<QSdRngTypes, QString>  m_rngTypesMap;
    QStringList m_rngTypesList;
    void fillRngTypes();

    QMap<QSdSampleTypes, QString> m_sampleTypeMap;
    QStringList m_sdSampleTypeList;
    void fillSamplerTypes();

    QMap<QSdSchedulerTypes, QString> m_schedulerTypesMap;
    QStringList m_schedulerTypesList;
    void fillSchedulerTypes();

    QMap<QSdPredictionTypes, QString> m_predictionTypesMap;
    QStringList m_predictionTypesList;
    void fillPredictionTypes();

    QMap<QSdPreviewTypes, QString> m_previewTypesMap;
    QStringList m_previewTypesList;
    void fillPreviewTypes();

    QMap<QSdLoraApplyModeTypes, QString> m_loraApplyModeTypesMap;
    QStringList m_loraApplyModeTypesList;
    void fillLoraApplyModesTypes();

    QMap<QSdVaeFormatTypes, QString> m_vaeFormatTypesMap;
    QStringList m_vaeFormatTypesList;
    void fillVaeFormatTypes();

    QMap<QSdLogLevelTypes, QString> m_logLevelTypesMap;
    QStringList m_logLevelTypesList;
    void fillLogLevelTypes();

    QMap<QSdCacheModeTypes, QString> m_cacheModeTypesMap;
    QStringList m_cacheModeTypesList;
    void fillCacheModeTypes();

    QMap<QSdHiResUpscalerTypes, QString> m_hiResUpscalerTypesMap;
    QStringList m_hiResUpscalerTypesList;
    void fillHiResUpscalerTypes();
};



#endif // QSDENUMS_H
