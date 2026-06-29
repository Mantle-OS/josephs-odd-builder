#include "qsdsampleparams.h"

QSdSampleParams::QSdSampleParams(QObject *parent) :
    m_guidance{new QSdGuidanceParams{this}},
    QSdBaseParam{parent}
{
    resetSampleParams();
}

QSdSampleParams::~QSdSampleParams()
{
    resetSampleParams();
    if(m_guidance){
        delete m_guidance;
        m_guidance = nullptr;
    }
}

sd_sample_params_t QSdSampleParams::sampleParams()
{
    sd_sample_params_t ret= {};
    ret.guidance = m_guidance->guidanceParams();
    ret.scheduler = QSdEnums::sdSchedulerType(m_scheduler);
    ret.sample_method = QSdEnums::sdSampleType(m_sampleMethod);
    ret.sample_steps = m_sampleSteps;
    ret.eta = m_eta;
    ret.shifted_timestep = m_shiftedTimestep;
    if(!m_customSigmas.isEmpty()){
        m_proxySigmas.clear();
        m_proxySigmas.resize(m_customSigmas.size());

        for(float i : m_customSigmas.toList()){
            m_proxySigmas.push_back(i);
        }
        ret.custom_sigmas       = m_proxySigmas.data();
        ret.custom_sigmas_count = m_proxySigmas.size();
    }else{
        ret.custom_sigmas       = nullptr;
        ret.custom_sigmas_count = 0;
    }

    ret.flow_shift              = m_flowShift;

    if(!m_extraSampleArgs.isEmpty()){
        tmp_extraSampleArgs = m_extraSampleArgs.toLocal8Bit();
        ret.extra_sample_args = tmp_extraSampleArgs.constData();
    }else{
        tmp_extraSampleArgs = nullptr;
    }
    return ret;

}

void QSdSampleParams::setSampleParams(sd_sample_params_t other)
{
    m_guidance->setGuidanceParams(other.guidance);
    set_scheduler(QSdEnums::qsdSchedulerType(other.scheduler));
    set_sampleMethod(QSdEnums::qsdSampleType(other.sample_method));
    set_sampleSteps(other.sample_steps);
    set_eta(other.eta);
    set_shiftedTimestep(other.shifted_timestep);
    clearCustomSigmas();
    if (other.custom_sigmas && other.custom_sigmas_count > 0) {
        for (size_t i = 0; i < other.custom_sigmas_count; ++i) {
            m_customSigmas.append(other.custom_sigmas[i]);
        }
    }

    set_flowShift(other.flow_shift);

    if(other.extra_sample_args){
        set_extraSampleArgs(QString::fromLatin1(other.extra_sample_args));
    }else{
        set_extraSampleArgs("");
    }
    m_sampleParams = other;
}

void QSdSampleParams::resetSampleParams()
{
    m_sampleParams = {};
    m_sampleParams.guidance                    = m_guidance->guidanceParams();
    m_sampleParams.scheduler                   = SCHEDULER_COUNT;
    m_sampleParams.sample_method               = SAMPLE_METHOD_COUNT;
    m_sampleParams.sample_steps                = 20;
    m_sampleParams.eta                         = INFINITY;
    m_sampleParams.shifted_timestep            = 0;
    m_sampleParams.custom_sigmas               = nullptr;
    m_sampleParams.custom_sigmas_count         = 0;
    m_sampleParams.flow_shift                  = INFINITY;
    m_sampleParams.extra_sample_args           = nullptr;

    setSampleParams(m_sampleParams);
}


QList<float> QSdSampleParams::customSigmas() const
{
    return m_customSigmas;
}

void QSdSampleParams::appendCustomSigma(float sigmas)
{
    m_customSigmas.append(sigmas);
    Q_EMIT customSigmasChanged();
}

void QSdSampleParams::prependCustomSigma(float sigmas)
{
    m_customSigmas.prepend(sigmas);
    Q_EMIT customSigmasChanged();
}

void QSdSampleParams::clearCustomSigmas()
{
    m_customSigmas.clear();
    m_proxySigmas.clear();
    Q_EMIT customSigmasChanged();
}

QString QSdSampleParams::debugString()
{
    QString ret = "Unknown";
    auto dd = sampleParams();
    ret = QString::fromLatin1(sd_sample_params_to_str(&dd));
    return ret;
}