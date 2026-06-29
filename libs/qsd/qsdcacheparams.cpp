#include "qsdcacheparams.h"

QSdCacheParams::QSdCacheParams(QObject *parent) :
    QSdBaseParam{parent}
{
    resetCacheParams();
}

QSdCacheParams::~QSdCacheParams()
{
    resetCacheParams();
}

sd_cache_params_t QSdCacheParams::cacheParams()
{

    sd_cache_params_t ret{};
    ret.mode                        = QSdEnums::sdCacheModeType(m_mode);
    ret.reuse_threshold             = m_reuseThreshold;
    ret.start_percent               = m_startPercent;
    ret.end_percent                 = m_endPercent;
    ret.error_decay_rate            = m_errorDecayRate;
    ret.use_relative_threshold      = m_useRelativeThreshold;
    ret.reset_error_on_compute      = m_resetErrorOnCompute;
    ret.Fn_compute_blocks           = m_fnComputeBlocks;
    ret.Bn_compute_blocks           = m_bnComputeBlocks;
    ret.residual_diff_threshold     = m_residualDiffThreshold;
    ret.max_warmup_steps            = m_maxWarmupSteps;
    ret.max_cached_steps            = m_maxCachedSteps;
    ret.max_continuous_cached_steps = m_maxContinuousCachedSteps;
    ret.taylorseer_n_derivatives    = m_taylorseerNDerivatives;
    ret.taylorseer_skip_interval    = m_taylorseerSkipInterval;

    if (!m_scmMask.isEmpty()){
        tmp_scmMask = m_scmMask.toLocal8Bit();
        ret.scm_mask                = tmp_scmMask.constData();
    }else{
        ret.scm_mask                = nullptr;
    }

    ret.scm_policy_dynamic          = m_scmPolicyDynamic;
    ret.spectrum_w                  = m_spectrumW;
    ret.spectrum_m                  = m_spectrumM;
    ret.spectrum_lam                = m_spectrumLam;
    ret.spectrum_window_size        = m_spectrumWindowSize;
    ret.spectrum_flex_window        = m_spectrumFlexWindow;
    ret.spectrum_warmup_steps       = m_spectrumWarmupSteps;
    ret.spectrum_stop_percent       = m_spectrumStopPercent;

    return ret;
}

void QSdCacheParams::setCacheParams(sd_cache_params_t other)
{
    set_mode(QSdEnums::qsdCacheModeType(other.mode));
    set_reuseThreshold(other.reuse_threshold);
    set_startPercent(other.start_percent);
    set_endPercent(other.end_percent);
    set_errorDecayRate(other.error_decay_rate);
    set_useRelativeThreshold(other.use_relative_threshold);
    set_resetErrorOnCompute(other.reset_error_on_compute);
    set_fnComputeBlocks(other.Fn_compute_blocks);
    set_bnComputeBlocks(other.Bn_compute_blocks);
    set_residualDiffThreshold(other.residual_diff_threshold);
    set_maxWarmupSteps(other.max_warmup_steps);
    set_maxCachedSteps(other.max_cached_steps);
    set_maxContinuousCachedSteps(other.max_continuous_cached_steps);
    set_taylorseerNDerivatives(other.taylorseer_n_derivatives);
    set_taylorseerSkipInterval(other.taylorseer_skip_interval);

    if(other.scm_mask){
        set_scmMask(QString::fromLatin1(other.scm_mask));
    }else{
        set_scmMask("");
    }

    set_scmPolicyDynamic(other.scm_policy_dynamic);
    set_spectrumW(other.spectrum_w);
    set_spectrumM(other.spectrum_m);
    set_spectrumLam(other.spectrum_lam);
    set_spectrumWindowSize(other.spectrum_window_size);
    set_spectrumFlexWindow(other.spectrum_flex_window);
    set_spectrumWarmupSteps(other.spectrum_warmup_steps);
    set_spectrumStopPercent(other.spectrum_stop_percent);
    m_cacheParams = other;
}

void QSdCacheParams::resetCacheParams()
{
    m_cacheParams.mode                        = SD_CACHE_DISABLED;
    m_cacheParams.reuse_threshold             = INFINITY;
    m_cacheParams.start_percent               = 0.15f;
    m_cacheParams.end_percent                 = 0.95f;
    m_cacheParams.error_decay_rate            = 1.0f;
    m_cacheParams.use_relative_threshold      = true;
    m_cacheParams.reset_error_on_compute      = true;
    m_cacheParams.Fn_compute_blocks           = 8;
    m_cacheParams.Bn_compute_blocks           = 0;
    m_cacheParams.residual_diff_threshold     = 0.08f;
    m_cacheParams.max_warmup_steps            = 8;
    m_cacheParams.max_cached_steps            = -1;
    m_cacheParams.max_continuous_cached_steps = -1;
    m_cacheParams.taylorseer_n_derivatives    = 1;
    m_cacheParams.taylorseer_skip_interval    = 1;
    m_cacheParams.scm_mask                    = nullptr;
    m_cacheParams.scm_policy_dynamic          = true;
    m_cacheParams.spectrum_w                  = 0.40f;
    m_cacheParams.spectrum_m                  = 3;
    m_cacheParams.spectrum_lam                = 1.0f;
    m_cacheParams.spectrum_window_size        = 2;
    m_cacheParams.spectrum_flex_window        = 0.50f;
    m_cacheParams.spectrum_warmup_steps       = 4;
    m_cacheParams.spectrum_stop_percent       = 0.9f;
    setCacheParams(m_cacheParams);
}

