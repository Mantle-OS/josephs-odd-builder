#include "qsdenums.h"
QSdEnums::QSdEnums(QObject *parent) :
    QObject{parent}
{
    fillAllTypes();
}

QSdEnums::~QSdEnums()
{
    clearAllTypes();
}


sd_type_t QSdEnums::sdWeightType(const QSdWeightTypes &sdType)
{
    return  static_cast<sd_type_t>(sdType);
}

QString QSdEnums::weightTypeName(QSdWeightTypes type)
{
    if(m_weightTypesMap.contains(type))
        return m_weightTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdWeightTypes QSdEnums::weightTypeFromString(const QString &type)
{

    QMapIterator<QSdWeightTypes, QString> it(m_weightTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdWeightTypes::QSdCount;
}

QSdEnums::QSdWeightTypes QSdEnums::qsdWeightType(const sd_type_t &type)
{
    return static_cast<QSdWeightTypes>(type);
}

int QSdEnums::weightTypeIndexAt(const QString &in)
{
    int ret = 0;
    QMapIterator<QSdWeightTypes, QString> it(m_weightTypesMap);
    while(it.hasNext()){
        it.next();
        if(in == it.value())
            return static_cast<int>(it.key());
    }
    return ret;
}

int QSdEnums::weightTypeIndexAt(const QSdWeightTypes &sdType)
{
    return static_cast<int>(sdType);
}

QStringList QSdEnums::weightTypesList() const
{
    return m_weightTypesList;
}

QStringList QSdEnums::rngTypesList() const
{
    return m_rngTypesList;
}

rng_type_t QSdEnums::sdRngType(const QSdRngTypes rngType)
{
    return static_cast<rng_type_t>(rngType);
}

QString QSdEnums::rngTypeName(QSdRngTypes type)
{
    if(m_rngTypesMap.contains(type))
        return m_rngTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdRngTypes QSdEnums::rngTypeFromString(const QString &type)
{
    QMapIterator<QSdRngTypes, QString> it(m_rngTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdRngTypes::QSdRngTypeCount;
}

QSdEnums::QSdRngTypes QSdEnums::qsdRngType(rng_type_t rngType)
{
    return static_cast<QSdRngTypes>(rngType);
}

QStringList QSdEnums::sampleTypesList() const
{
    return m_sdSampleTypeList;
}

sample_method_t QSdEnums::sdSampleType(const QSdSampleTypes &type)
{
    return static_cast<sample_method_t>(type);
}

QSdEnums::QSdSampleTypes QSdEnums::qsdSampleType(sample_method_t type)
{
    return static_cast<QSdSampleTypes>(type);
}

QString QSdEnums::sampleTypeName(QSdSampleTypes type)
{
    if(m_sampleTypeMap.contains(type))
        return m_sampleTypeMap[type];
    return "Unknown";
}

QSdEnums::QSdSampleTypes QSdEnums::sampleTypeFromString(const QString &type)
{
    QMapIterator<QSdSampleTypes, QString> it(m_sampleTypeMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdSampleTypes::QSdSampleCount;
}

QStringList QSdEnums::schedulerTypesList() const
{
    return m_schedulerTypesList;
}

scheduler_t QSdEnums::sdSchedulerType(const QSdSchedulerTypes &type)
{
    return static_cast<scheduler_t>(type);
}

QSdEnums::QSdSchedulerTypes QSdEnums::qsdSchedulerType(scheduler_t type)
{
    return static_cast<QSdSchedulerTypes>(type);
}

QString QSdEnums::schedulerTypeName(QSdSchedulerTypes type)
{
    if(m_schedulerTypesMap.contains(type))
        return m_schedulerTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdSchedulerTypes QSdEnums::schedulerTypeFromString(const QString &type)
{
    QMapIterator<QSdSchedulerTypes, QString> it(m_schedulerTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdSchedulerTypes::QSdSchedulerCount;
}

QStringList QSdEnums::predictionTypesList() const
{
    return m_predictionTypesList;
}

prediction_t QSdEnums::sdPredictionType(const QSdPredictionTypes &type)
{
    return static_cast<prediction_t>(type);
}

QString QSdEnums::schedulerTypeName(QSdPredictionTypes type)
{
    if(m_predictionTypesMap.contains(type))
        return m_predictionTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdPredictionTypes QSdEnums::predictionTypeFromString(const QString &type)
{
    QMapIterator<QSdPredictionTypes, QString> it(m_predictionTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdPredictionTypes::QSdPredictionCount;
}

QSdEnums::QSdPredictionTypes QSdEnums::qsdPredictionType(prediction_t type)
{
    return static_cast<QSdPredictionTypes>(type);
}

QStringList QSdEnums::previewTypesList() const
{
    return m_previewTypesList;
}

preview_t QSdEnums::sdPreviewType(const QSdPreviewTypes &type)
{
    return static_cast<preview_t>(type);
}

QSdEnums::QSdPreviewTypes QSdEnums::qsdPreviewType(preview_t type)
{
    return static_cast<QSdPreviewTypes>(type);
}

QString QSdEnums::previewTypeName(QSdPreviewTypes type)
{
    if(m_previewTypesMap.contains(type))
        return m_previewTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdPreviewTypes QSdEnums::previewTypeFromString(const QString &type)
{
    QMapIterator<QSdPreviewTypes, QString> it(m_previewTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdPreviewTypes::QSdPreviewCount;
}

QStringList QSdEnums::loraApplyModeTypesList() const
{
    return m_loraApplyModeTypesList;
}

lora_apply_mode_t QSdEnums::sdLoraApplyModeType(const QSdLoraApplyModeTypes &type)
{
    return static_cast<lora_apply_mode_t>(type);
}

QSdEnums::QSdLoraApplyModeTypes QSdEnums::qsdLoraApplyModeType(lora_apply_mode_t type)
{
    return static_cast<QSdLoraApplyModeTypes>(type);
}

QString QSdEnums::loraApplyModeTypeName(QSdLoraApplyModeTypes type)
{
    if(m_loraApplyModeTypesMap.contains(type))
        return m_loraApplyModeTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdLoraApplyModeTypes QSdEnums::loraApplyModeTypeFromString(const QString &type)
{
    QMapIterator<QSdLoraApplyModeTypes, QString> it(m_loraApplyModeTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdLoraApplyModeTypes::QSdLoraCount;
}

QStringList QSdEnums::vaeFormatTypesList() const
{
    return m_vaeFormatTypesList;
}

sd_vae_format_t QSdEnums::sdVaeFormatType(const QSdVaeFormatTypes &type)
{
    return static_cast<sd_vae_format_t>(type);
}

QSdEnums::QSdVaeFormatTypes QSdEnums::qsdVaeFormatType(sd_vae_format_t type)
{
    return static_cast<QSdVaeFormatTypes>(type);
}

QString QSdEnums::vaeFormatTypeName(QSdVaeFormatTypes type)
{
    if(m_vaeFormatTypesMap.contains(type))
        return m_vaeFormatTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdVaeFormatTypes QSdEnums::vaeFormatTypeFromString(const QString &type)
{
    QMapIterator<QSdVaeFormatTypes, QString> it(m_vaeFormatTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdVaeFormatTypes::QSdVaeFormatCount;
}

QStringList QSdEnums::logLevelTypesList() const
{
    return m_logLevelTypesList;
}

sd_log_level_t QSdEnums::sdLogLevelType(const QSdLogLevelTypes &type)
{
    return static_cast<sd_log_level_t>(type);
}

QSdEnums::QSdLogLevelTypes QSdEnums::qsdLogLevelType(sd_log_level_t type)
{
    return static_cast<QSdLogLevelTypes>(type);
}

QString QSdEnums::logLevelTypeName(QSdLogLevelTypes type)
{
    if(m_logLevelTypesMap.contains(type))
        return m_logLevelTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdLogLevelTypes QSdEnums::logLevelTypeFromString(const QString &type)
{
    QMapIterator<QSdLogLevelTypes, QString> it(m_logLevelTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdLogLevelTypes::QSdLogInfo;
}

QStringList QSdEnums::cacheModeTypesList() const
{
    return m_cacheModeTypesList;
}

QSdEnums::QSdCacheModeTypes QSdEnums::qsdCacheModeType(sd_cache_mode_t type)
{
    return static_cast<QSdCacheModeTypes>(type);
}

sd_cache_mode_t QSdEnums::sdCacheModeType(const QSdCacheModeTypes &type)
{
    return static_cast<sd_cache_mode_t>(type);
}

QString QSdEnums::cacheModeTypeName(QSdCacheModeTypes type)
{
    if(m_cacheModeTypesMap.contains(type))
        return m_cacheModeTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdCacheModeTypes QSdEnums::cacheModeTypeFromString(const QString &type)
{
    QMapIterator<QSdCacheModeTypes, QString> it(m_cacheModeTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdCacheModeTypes::QSdCacheDisabled;
}

QStringList QSdEnums::hiResUpscalerTypesList() const
{
    return m_hiResUpscalerTypesList;
}

sd_hires_upscaler_t QSdEnums::sdHiResUpscalerType(const QSdHiResUpscalerTypes &type)
{
    return static_cast<sd_hires_upscaler_t>(type);
}

QSdEnums::QSdHiResUpscalerTypes QSdEnums::qsdHiResUpscalerType(sd_hires_upscaler_t type)
{
    return static_cast<QSdHiResUpscalerTypes>(type);
}

QString QSdEnums::hiResUpscalerTypeName(QSdHiResUpscalerTypes type)
{
    if(m_hiResUpscalerTypesMap.contains(type))
        return m_hiResUpscalerTypesMap[type];
    return "Unknown";
}

QSdEnums::QSdHiResUpscalerTypes QSdEnums::hiResUpscalerTypeFromString(const QString &type)
{
    QMapIterator<QSdHiResUpscalerTypes, QString> it(m_hiResUpscalerTypesMap);
    while(it.hasNext()){
        it.next();
        if(type == it.value())
            return it.key();
    }
    return QSdHiResUpscalerTypes::QSdHiResUpscalerCount;
}

void QSdEnums::fillAllTypes()
{
    clearAllTypes();
    fillWeightTypes();
    fillRngTypes();
    fillSamplerTypes();
    fillSchedulerTypes();
    fillPredictionTypes();
    fillPreviewTypes();
    fillLoraApplyModesTypes();
    fillVaeFormatTypes();
    fillLogLevelTypes();
    fillCacheModeTypes();
    fillHiResUpscalerTypes();
}

void QSdEnums::clearAllTypes()
{
    m_weightTypesMap.clear();
    m_weightTypesList.clear();

    m_rngTypesMap.clear();
    m_rngTypesList.clear();

    m_sampleTypeMap.clear();
    m_sdSampleTypeList.clear();

    m_schedulerTypesMap.clear();
    m_schedulerTypesList.clear();

    m_predictionTypesMap.clear();
    m_predictionTypesList.clear();

    m_previewTypesMap.clear();
    m_previewTypesList.clear();

    m_loraApplyModeTypesMap.clear();
    m_loraApplyModeTypesList.clear();

    m_vaeFormatTypesMap.clear();
    m_vaeFormatTypesList.clear();

    m_logLevelTypesMap.clear();
    m_logLevelTypesList.clear();

    m_cacheModeTypesMap.clear();
    m_cacheModeTypesList.clear();

    m_hiResUpscalerTypesMap.clear();
    m_hiResUpscalerTypesList.clear();
}

void QSdEnums::fillWeightTypes()
{
    m_weightTypesMap = {
                        {QSdF32,      QStringLiteral("F32")},
                        {QSdF16,      QStringLiteral("F16")},
                        {QSdQ4_0,     QStringLiteral("Q4_0")},
                        {QSdQ4_1,     QStringLiteral("Q4_1")},
                        // SD_TYPE_Q4_2 = 4, support has been removed
                        // SD_TYPE_Q4_3 = 5, support has been removed
                        {QSdQ5_0,     QStringLiteral("Q5_0")},
                        {QSdQ5_1,     QStringLiteral("Q5_1")},
                        {QSdQ8_0,     QStringLiteral("Q8_0")},
                        {QSdQ8_1,     QStringLiteral("Q8_1")},
                        {QSdQ2_K,     QStringLiteral("Q2_K")},
                        {QSdQ3_K,     QStringLiteral("Q3_K")},
                        {QSdQ4_K,     QStringLiteral("Q4_K")},
                        {QSdQ5_K,     QStringLiteral("Q5_K")},
                        {QSdQ6_K,     QStringLiteral("Q6_K")},
                        {QSdQ8_K,     QStringLiteral("Q8_K")},
                        {QSdIQ2_XXS,  QStringLiteral("IQ2_XXS")},
                        {QSdIQ2_XS,   QStringLiteral("IQ2_XS")},
                        {QSdIQ3_XXS,  QStringLiteral("IQ3_XXS")},
                        {QSdIQ1_S,    QStringLiteral("IQ1_S")},
                        {QSdIQ4_NL,   QStringLiteral("IQ4_NL")},
                        {QSdIQ3_S,    QStringLiteral("IQ3_S")},
                        {QSdIQ2_S,    QStringLiteral("IQ2_S")},
                        {QSdIQ4_XS,   QStringLiteral("IQ4_XS")},
                        {QSdI8,       QStringLiteral("I8")},
                        {QSdI16,      QStringLiteral("I16")},
                        {QSdI32,      QStringLiteral("I32")},
                        {QSdI64,      QStringLiteral("I64")},
                        {QSdF64,      QStringLiteral("F64")},
                        {QSdIQ1_M,    QStringLiteral("IQ1_M")},
                        {QSdBF16,     QStringLiteral("BF16")},
                        // SD_TYPE_Q4_0_4_4 = 31, support has been removed from gguf files
                        // SD_TYPE_Q4_0_4_8 = 32,
                        // SD_TYPE_Q4_0_8_8 = 33,
                        {QSdTQ1_0,    QStringLiteral("TQ1_0")},
                        {QSdTQ2_0,    QStringLiteral("TQ2_0")},
                        // SD_TYPE_IQ4_NL_4_4 = 36,
                        // SD_TYPE_IQ4_NL_4_8 = 37,
                        // SD_TYPE_IQ4_NL_8_8 = 38,
                        {QSdMXFP4,    QStringLiteral("MXFP4")},
                        {QSdNVFP4,    QStringLiteral("NVFP4")},
                        {QSdQ1_0,     QStringLiteral("Q1_0")},
                        {QSdCount,    QStringLiteral("COUNT")},
                        };
    // goal iter over all the new m_typeMap and setup a QStringList for Qml View (comboBoxes)
    QMapIterator<QSdWeightTypes, QString> typeIt(m_weightTypesMap);
    while(typeIt.hasNext()){
        typeIt.next();
        m_weightTypesList.append(typeIt.value());
    }
    Q_EMIT weightTypesListChanged();

}

void QSdEnums::fillRngTypes(){
    m_rngTypesMap = {
                     {QSdStdDefaultRng,     QStringLiteral("StdDefaultRng")},
                     {QSdCudaRNG,           QStringLiteral("CudaRNG")},
                     {QSdCpuRNG,            QStringLiteral("CpuRNG")},
                     {QSdRngTypeCount,      QStringLiteral("RngTypeCount")},
                     };
    QMapIterator<QSdRngTypes, QString> it(m_rngTypesMap);
    while(it.hasNext()){
        it.next();
        m_rngTypesList.append(it.value());
    }
    Q_EMIT rngTypesListChanged();
}

void QSdEnums::fillSamplerTypes()
{
    m_sampleTypeMap = {
                       { QSdEuler,          QStringLiteral("Euler") },
                       { QSdEulerA,         QStringLiteral("Euler A")},
                       { QSdHeun,           QStringLiteral("Heun")},
                       { QSdDpm2,           QStringLiteral("Dpm2")},
                       { QSdDpmPP2SA,       QStringLiteral("Dpm Pp2sa")},
                       { QSdDpmPP2M,        QStringLiteral("Dpm Pp2m")},
                       { QSdDpmPP2Mv2,      QStringLiteral("Dpm Pp2m v2") },
                       { QSdIpndm,          QStringLiteral("Ipndm")},
                       { QSdIpndmV,         QStringLiteral("Ipndm V") },
                       { QSdLcmSampler,     QStringLiteral("Lcm") },
                       { QSdDddmTrailing,   QStringLiteral("Dddm Trailing") },
                       { QSdTcd,            QStringLiteral("Tcd") },
                       { QSdResMultistep,   QStringLiteral("Res Multistep")},
                       { QSdRes2S,          QStringLiteral("Res 2s") },
                       { QSdErSde,          QStringLiteral("Er Sde") },
                       { QSdEulerCfgPp,     QStringLiteral("Euler Cfg pp")},
                       { QSdEulerACfpPp,    QStringLiteral("Euler Acfp pp")},
                       { QSdEulerGe,        QStringLiteral("Euler Ge")},
                       { QSdSampleCount,    QStringLiteral("Count")},
                       };
    QMapIterator<QSdSampleTypes, QString> it(m_sampleTypeMap);
    while(it.hasNext()){
        it.next();
        m_sdSampleTypeList.append(it.value());
    }
    Q_EMIT sampleTypesListChanged();
}

void QSdEnums::fillSchedulerTypes()
{
    m_schedulerTypesMap = {
                           { QSdDiscrete,       QStringLiteral("Discrete") },
                           { QSdKarras,         QStringLiteral("Karras") },
                           { QSdExponential,    QStringLiteral("Exponential") },
                           { QSdAys,            QStringLiteral("Ays") },
                           { QSdGits,           QStringLiteral("Gits") },
                           { QSdSgmUniform,     QStringLiteral("Sgm Uniform") },
                           { QSdSimple,         QStringLiteral("Simple") },
                           { QSdSmoothstep,     QStringLiteral("Smoothstep") },
                           { QSdKlOptimal,      QStringLiteral("Kl Optimal") },
                           { QSdLcmScheduler,   QStringLiteral("Lcm") },
                           { QSdBong,           QStringLiteral("Bong") },
                           { QSdLtx2,           QStringLiteral("Ltx2.3") },
                           { QSdSchedulerCount, QStringLiteral("Count") },
                           };
    QMapIterator<QSdSchedulerTypes, QString> it(m_schedulerTypesMap);
    while(it.hasNext()){
        it.next();
        m_schedulerTypesList.append(it.value());
    }
    Q_EMIT schedulerTypesListChanged();
}

void QSdEnums::fillPredictionTypes()
{
    m_predictionTypesMap = {
                            { QSdEps,               QStringLiteral("Eps")},
                            { QSdV,                 QStringLiteral("V")},
                            { QSdEdmV,              QStringLiteral("Edm V")},
                            { QSdFlow,              QStringLiteral("Flow")},
                            { QSdFluxFlow,          QStringLiteral("Flux")},
                            { QSdFlux2Flow,         QStringLiteral("Flux 2")},
                            { QSdPredictionCount,   QStringLiteral("Count")},
                            };
    QMapIterator<QSdPredictionTypes, QString> it(m_predictionTypesMap);
    while(it.hasNext()){
        it.next();
        m_predictionTypesList.append(it.value());
    }
    Q_EMIT predictionTypesListChanged();
}

void QSdEnums::fillPreviewTypes()
{
    m_previewTypesMap = {
                         { QSdPreviewNone,     QStringLiteral("None")},
                         { QSdPreviewProj,     QStringLiteral("Proj")},
                         { QSdPreviewTae,      QStringLiteral("Tae")},
                         { QSdPreviewVae,      QStringLiteral("Vae")},
                         { QSdPreviewCount,    QStringLiteral("Count")},
                         };
    QMapIterator<QSdPreviewTypes, QString> it(m_previewTypesMap);
    while(it.hasNext()){
        it.next();
        m_previewTypesList.append(it.value());
    }
    Q_EMIT previewTypesListChanged();
}

void QSdEnums::fillLoraApplyModesTypes()
{
    m_loraApplyModeTypesMap = {
                               { QSdLoraAuto,           QStringLiteral("Auto")},
                               { QSdLoraImmediately,    QStringLiteral("Immediately")},
                               { QSdLoraAtRuntime,      QStringLiteral("Runtime")},
                               { QSdLoraCount,          QStringLiteral("Count")},
                               };

    QMapIterator<QSdLoraApplyModeTypes, QString> it(m_loraApplyModeTypesMap);
    while(it.hasNext()){
        it.next();
        m_loraApplyModeTypesList.append(it.value());
    }
    Q_EMIT loraApplyModeTypesListChanged();
}

void QSdEnums::fillVaeFormatTypes()
{
    m_vaeFormatTypesMap = {
                           { QSdVaeFormatAuto,     QStringLiteral("Auto")},
                           { QSdVaeFormatFlux,     QStringLiteral("Flux")},
                           { QSdVaeFormatSd3,      QStringLiteral("Sd3")},
                           { QSdVaeFormatFlux2,    QStringLiteral("Flux2")},
                           { QSdVaeFormatCount,    QStringLiteral("Count")},
                           };

    QMapIterator<QSdVaeFormatTypes, QString> it(m_vaeFormatTypesMap);
    while(it.hasNext()){
        it.next();
        m_vaeFormatTypesList.append(it.value());
    }
    Q_EMIT vaeFormatTypesListChanged();
}

void QSdEnums::fillLogLevelTypes()
{
    m_logLevelTypesMap = {
                          { QSdLogDebug,    QStringLiteral("Debug")},
                          { QSdLogInfo,     QStringLiteral("Info")},
                          { QSdLogWarn,     QStringLiteral("Warn")},
                          { QSdLogError,    QStringLiteral("Error")},
                          };

    QMapIterator<QSdLogLevelTypes, QString> it(m_logLevelTypesMap);
    while(it.hasNext()){
        it.next();
        m_logLevelTypesList.append(it.value());
    }
    Q_EMIT logLevelTypesListChanged();
}

void QSdEnums::fillCacheModeTypes()
{
    m_cacheModeTypesMap = {
                           { QSdCacheDisabled,   QStringLiteral("Disabled")},
                           { QSdCacheEasycache,   QStringLiteral("Easy Cache")},
                           { QSdCacheUcache,   QStringLiteral("U Cache")},
                           { QSdCacheDbcache,   QStringLiteral("Db Cache")},
                           { QSdCacheTaylorseer,   QStringLiteral("Taylorseer")},
                           { QSdCacheCacheDit,   QStringLiteral("Dit")},
                           { QSdCacheSpectrum,   QStringLiteral("Spectrum")},
                           };

    QMapIterator<QSdCacheModeTypes, QString> it(m_cacheModeTypesMap);
    while(it.hasNext()){
        it.next();
        m_cacheModeTypesList.append(it.value());
    }
    Q_EMIT cacheModeTypesListChanged();
}

void QSdEnums::fillHiResUpscalerTypes()
{
    m_hiResUpscalerTypesMap = {
                               { QSdHiResUpscalerNone,                      QStringLiteral("None")},
                               { QSdHiResUpscalerLatent,                    QStringLiteral("Latent")},
                               { QSdHiResUpscalerLatentNearest,             QStringLiteral("Nearest")},
                               { QSdHiResUpscalerLatentNearestExact,        QStringLiteral("Nearest Exact")},
                               { QSdHiResUpscalerLatentAntialiased,         QStringLiteral("Antialiased")},
                               { QSdHiResUpscalerLatentBicubic,             QStringLiteral("Bicubic")},
                               { QSdHiResUpscalerLatentBicubicAntialiased,  QStringLiteral("Bicubic Antialiased")},
                               { QSdHiResUpscalerLanczos,                   QStringLiteral("Lanczos")},
                               { QSdHiResUpscalerNearest,                   QStringLiteral("Nearest")},
                               { QSdHiResUpscalerModel,                     QStringLiteral("Model")},
                               { QSdHiResUpscalerCount,                     QStringLiteral("Count")},
                               };

    QMapIterator<QSdHiResUpscalerTypes, QString> it(m_hiResUpscalerTypesMap);
    while(it.hasNext()){
        it.next();
        m_hiResUpscalerTypesList.append(it.value());
    }
    Q_EMIT hiResUpscalerTypesListChanged();
}

