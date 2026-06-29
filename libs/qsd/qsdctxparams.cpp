#include "qsdctxparams.h"

QSdCtxParams::QSdCtxParams(QObject *parent) :
    m_embeddings{new ObjectListModel<QSdEmbedding>{this, "", ""}},
    QSdBaseParam{parent}
{
}

QSdCtxParams::~QSdCtxParams()
{
    if(!m_embeddings->isEmpty())
        m_embeddings->clear();
}

sd_ctx_params_t QSdCtxParams::ctxParams()
{
    sd_ctx_params_t sd_ctx_params;
    sd_ctx_params_init(&sd_ctx_params);

    if(m_weightsOnCpu){
        if(!m_paramsBackend.contains("*=cpu"))
            m_paramsBackend.prepend("*=cpu,");
    }
    if(m_clipOnCpu){
        if(!m_backend.contains("te=cpu"))
            m_backend.prepend("te=cpu,");
    }

    if(m_vaeOnCpu){
        if(!m_backend.contains("vae=cpu"))
            m_backend.prepend("vae=cpu,");
    }

    if(m_controlNetOnCpu){
        if(!m_backend.contains("controlnet=cpu"))
            m_backend.prepend("controlnet=cpu,");
    }

    if(!m_embeddings->isEmpty()){
        m_embeddingVec.clear();
        m_embeddingVec.reserve(m_embeddings->size());
        std::string name = {};
        std::string path = {};
        for(auto *i : m_embeddings->toList()){
            sd_embedding_t item;
            if(i->get_isEnabled()){
                name = i->get_embeddingName().toStdString();
                path = i->get_embeddingPath().toStdString();
                item.name = name.c_str();
                item.path = path.c_str();
                m_embeddingVec.emplace_back(item);
            }
        }
    }

    if(!m_modelPath.isEmpty() && QAiUtils::fileExists(m_modelPath)){
        tmp_modelPath = m_modelPath.toLocal8Bit();
        sd_ctx_params.model_path  = tmp_modelPath.constData();
    } else {
        sd_ctx_params.model_path  = nullptr;
    }

    if(!m_clipLPath.isEmpty() && QAiUtils::fileExists(m_clipLPath)){
        tmp_clipLPath = m_clipLPath.toLocal8Bit();
        sd_ctx_params.clip_l_path = tmp_clipLPath.constData();
    } else {
        sd_ctx_params.clip_l_path = nullptr;
    }

    if(!m_clipGPath.isEmpty() && QAiUtils::fileExists(m_clipGPath)){
        tmp_clipGPath = m_clipGPath.toLocal8Bit();
        sd_ctx_params.clip_g_path = tmp_clipGPath.constData();
    } else {
        sd_ctx_params.clip_g_path = nullptr;
    }

    if(!m_clipVisionPath.isEmpty() && QAiUtils::fileExists(m_clipVisionPath)){
        tmp_clipVisionPath = m_clipVisionPath.toLocal8Bit();
        sd_ctx_params.clip_vision_path = tmp_clipVisionPath.constData();
    } else {
        sd_ctx_params.clip_vision_path = nullptr;
    }

    if(!m_t5xxlPath.isEmpty() && QAiUtils::fileExists(m_t5xxlPath)){
        tmp_t5xxlPath = m_t5xxlPath.toLocal8Bit();
        sd_ctx_params.t5xxl_path = tmp_t5xxlPath.constData();
    } else {
        sd_ctx_params.t5xxl_path = nullptr;
    }

    if(!m_llmPath.isEmpty() && QAiUtils::fileExists(m_llmPath)){
        tmp_llmPath = m_llmPath.toLocal8Bit();
        sd_ctx_params.llm_path = tmp_llmPath.constData();
    } else {
        sd_ctx_params.llm_path = nullptr;
    }

    if(!m_llmVisionPath.isEmpty() && QAiUtils::fileExists(m_llmVisionPath)){
        tmp_llmVisionPath = m_llmVisionPath.toLocal8Bit();
        sd_ctx_params.llm_vision_path = tmp_llmVisionPath.constData();
    } else {
        sd_ctx_params.llm_vision_path = nullptr;
    }

    if(!m_diffusionModelPath.isEmpty() && QAiUtils::fileExists(m_diffusionModelPath)){
        tmp_diffusionModelPath = m_diffusionModelPath.toLocal8Bit();
        sd_ctx_params.diffusion_model_path = tmp_diffusionModelPath.constData();
    } else {
        sd_ctx_params.diffusion_model_path = nullptr;
    }

    if(!m_highNoiseDiffusionModelPath.isEmpty() && QAiUtils::fileExists(m_highNoiseDiffusionModelPath)){
        tmp_highNoiseDiffusionModelPath = m_highNoiseDiffusionModelPath.toLocal8Bit();
        sd_ctx_params.high_noise_diffusion_model_path = tmp_highNoiseDiffusionModelPath.constData();
    } else {
        sd_ctx_params.high_noise_diffusion_model_path = nullptr;
    }

    if(!m_uncondDiffusionModelPath.isEmpty() && QAiUtils::fileExists(m_uncondDiffusionModelPath)){
        tmp_uncondDiffusionModelPath = m_uncondDiffusionModelPath.toLocal8Bit();
        sd_ctx_params.uncond_diffusion_model_path = tmp_uncondDiffusionModelPath.constData();
    }else{
        sd_ctx_params.uncond_diffusion_model_path = nullptr;
    }

    if(!m_embeddingsConnectorsPath.isEmpty() && QAiUtils::fileExists(m_embeddingsConnectorsPath)){
        tmp_embeddingsConnectorsPath = m_embeddingsConnectorsPath.toLocal8Bit();
        sd_ctx_params.embeddings_connectors_path = tmp_embeddingsConnectorsPath.constData();
    }else{
        sd_ctx_params.embeddings_connectors_path = nullptr;
    }

    if(!m_vaePath.isEmpty() && QAiUtils::fileExists(m_vaePath)){
        tmp_vaePath = m_vaePath.toLocal8Bit();
        sd_ctx_params.vae_path = tmp_vaePath.constData();
    }else{
        sd_ctx_params.vae_path = nullptr;
    }

    if(!m_audioVaePath.isEmpty() && QAiUtils::fileExists(m_audioVaePath)){
        m_audioVaePath = m_audioVaePath.toLocal8Bit();
        sd_ctx_params.audio_vae_path = tmp_audioVaePath.constData();
    } else {
        sd_ctx_params.audio_vae_path = nullptr;
    }

    if(!m_taesdPath.isEmpty() && QAiUtils::fileExists(m_taesdPath)){
        tmp_taesdPath = m_taesdPath.toLocal8Bit();
        sd_ctx_params.taesd_path = tmp_taesdPath.constData();
    }else{
        sd_ctx_params.taesd_path = nullptr;
    }

    if(!m_controlNetPath.isEmpty() && QAiUtils::fileExists(m_controlNetPath)){
        tmp_controlNetPath = m_controlNetPath.toLocal8Bit();
        sd_ctx_params.taesd_path = tmp_controlNetPath.constData();
    }else{
        sd_ctx_params.taesd_path = nullptr;
    }

    if(!m_photoMakerPath.isEmpty() && QAiUtils::fileExists(m_photoMakerPath)){
        tmp_photoMakerPath = m_photoMakerPath.toLocal8Bit();
        sd_ctx_params.photo_maker_path = tmp_photoMakerPath.constData();
    }else{
        sd_ctx_params.photo_maker_path = nullptr;
    }

    sd_ctx_params.embeddings = m_embeddingVec.data();
    sd_ctx_params.embedding_count = static_cast<uint32_t>(m_embeddingVec.size());

    if(!m_tensorTypeRules.isEmpty()){
        tmp_tensorTypeRules = m_tensorTypeRules.toLocal8Bit();
        sd_ctx_params.tensor_type_rules = tmp_tensorTypeRules.constData();
    }

    if(m_numberOfThreads >= 0 && m_numberOfThreads <= sd_get_num_physical_cores()){
        sd_ctx_params.n_threads =  m_numberOfThreads;
    }else{
        sd_ctx_params.n_threads = (sd_get_num_physical_cores() - 1);
    }

    sd_ctx_params.wtype                             = QSdEnums::sdWeightType(m_weightType);
    sd_ctx_params.rng_type                          = QSdEnums::sdRngType(m_rngType);
    sd_ctx_params.sampler_rng_type                  = QSdEnums::sdRngType(m_samplerRngType);
    sd_ctx_params.prediction                        = QSdEnums::sdPredictionType(m_prediction);
    sd_ctx_params.lora_apply_mode                   = QSdEnums::sdLoraApplyModeType(m_loraApplyMode);
    sd_ctx_params.vae_format                        = QSdEnums::sdVaeFormatType(m_vaeFormat);

    sd_ctx_params.enable_mmap                       = m_enableMmap;
    sd_ctx_params.flash_attn                        = m_flashAttn;
    sd_ctx_params.diffusion_flash_attn              = m_diffusionFlashAttn;
    sd_ctx_params.tae_preview_only                  = m_taePreviewOnly;
    sd_ctx_params.diffusion_conv_direct             = m_diffusionConvDirect;
    sd_ctx_params.vae_conv_direct                   = m_vaeConvDirect;
    sd_ctx_params.circular_x                        = m_circularX;
    sd_ctx_params.circular_y                        = m_circularY;
    sd_ctx_params.force_sdxl_vae_conv_scale         = m_forceSdxlVaeConvScale;
    sd_ctx_params.chroma_use_dit_mask               = m_chromaUseDitMask;
    sd_ctx_params.chroma_use_t5_mask                = m_chromaUseT5Mask;
    sd_ctx_params.chroma_t5_mask_pad                = m_chromaT5MaskPad;
    sd_ctx_params.qwen_image_zero_cond_t            = m_qwenImageZero;
    sd_ctx_params.stream_layers                     = m_streamLayers;  // Enable residency+prefetch streaming on top of --max-vram (no effect without --max-vram)
    // GiB budget for graph-cut segmented param offload (0 = disabled, -1 = auto free VRAM minus 1 GiB)
    if(!m_maxVram.isEmpty()){
        tmp_maxVram = m_maxVram.toLocal8Bit();
        sd_ctx_params.max_vram = tmp_maxVram.constData();
    }

    if(!m_backend.isEmpty()){

        tmp_backend = m_backend.toLocal8Bit();
        sd_ctx_params.backend = tmp_backend.constData();
    }

    if(!m_paramsBackend.isEmpty()){
        tmp_paramsBackend = m_paramsBackend.toLocal8Bit();
        sd_ctx_params.params_backend = tmp_paramsBackend.constData();
    }

    if(!m_rpcServers.isEmpty()){
        tmp_rpcServers = m_rpcServers.toLocal8Bit();
        sd_ctx_params.rpc_servers = tmp_rpcServers.constData();
    }

    m_ctxParams = sd_ctx_params;
    return m_ctxParams;
}

void QSdCtxParams::setCtxParams(sd_ctx_params_t *other)
{

    if(m_paramsBackend.contains("*=cpu"))
        set_weightsOnCpu(true);

    if(m_backend.contains("te=cpu"))
        set_clipOnCpu(true);

    if(m_backend.contains("vae=cpu"))
        set_vaeOnCpu(true);

    if(m_backend.contains("controlnet=cpu"))
        set_controlNetOnCpu(true);


    if(other->model_path)
        set_modelPath(QString::fromLatin1(other->model_path));
    if(other->clip_l_path)
        set_clipLPath(QString::fromLatin1(other->clip_l_path));
    if(other->clip_g_path)
        set_clipGPath(QString::fromLatin1(other->clip_g_path));
    if(other->clip_vision_path)
        set_clipVisionPath(QString::fromLatin1(other->clip_vision_path));
    if(other->llm_path)
        set_llmPath(QString::fromLatin1(other->llm_path));
    if(other->t5xxl_path)
        set_t5xxlPath(QString::fromLatin1(other->t5xxl_path));
    if(other->diffusion_model_path)
        set_diffusionModelPath(QString::fromLatin1(other->diffusion_model_path));
    if(other->high_noise_diffusion_model_path)
        set_highNoiseDiffusionModelPath(QString::fromLatin1(other->high_noise_diffusion_model_path));
    if(other->uncond_diffusion_model_path)
        set_uncondDiffusionModelPath(QString::fromLatin1(other->uncond_diffusion_model_path));
    if(other->embeddings_connectors_path)
        set_embeddingsConnectorsPath(QString::fromLatin1(other->embeddings_connectors_path));
    if(other->vae_path)
        set_vaePath(QString::fromLatin1(other->vae_path));
    if(other->audio_vae_path)
        set_audioVaePath(QString::fromLatin1(other->audio_vae_path));
    if(other->taesd_path)
        set_taesdPath(QString::fromLatin1(other->taesd_path));
    if(other->control_net_path)
        set_controlNetPath(QString::fromLatin1(other->control_net_path));
    if(other->photo_maker_path)
        set_photoMakerPath(QString::fromLatin1(other->photo_maker_path));
    if(other->tensor_type_rules)
        set_tensorTypeRules(QString::fromLatin1(other->tensor_type_rules));
    if(other->backend)
        set_backend(QString::fromLatin1(other->backend));
    if(other->params_backend)
        set_paramsBackend(QString::fromLatin1(other->params_backend));
    if(other->rpc_servers)
        set_rpcServers(QString::fromLatin1(other->rpc_servers));

    //
    set_weightType(QSdEnums::qsdWeightType(other->wtype));
    set_rngType(QSdEnums::qsdRngType(other->rng_type));
    set_samplerRngType(QSdEnums::qsdRngType(other->sampler_rng_type));
    set_prediction(QSdEnums::qsdPredictionType(other->prediction));
    set_loraApplyMode(QSdEnums::qsdLoraApplyModeType(other->lora_apply_mode));
    set_vaeFormat(QSdEnums::qsdVaeFormatType(other->vae_format));


    // Loop all the embeddings

    int eCnt = other->embedding_count;
    std::vector<sd_embedding_t> tmpEmbed;
    if(eCnt > 0){

    }




    // const sd_embedding_t* i = other->embeddings;
    // m_embeddings->setEmbeddings(other->embeddings);
    // if(m_embeddings){
    // }

    // set_embeddingCount(other->embedding_count);

    set_numberOfThreads(other->n_threads);
    set_chromaT5MaskPad(other->chroma_t5_mask_pad);
    set_maxVram(other->max_vram);
    set_enableMmap(other->enable_mmap);
    set_flashAttn(other->flash_attn);
    set_diffusionFlashAttn(other->diffusion_flash_attn);
    set_taePreviewOnly(other->tae_preview_only);
    set_diffusionConvDirect(other->diffusion_conv_direct);
    set_vaeConvDirect(other->vae_conv_direct);
    set_circularX(other->circular_x);
    set_circularY(other->circular_y);
    set_forceSdxlVaeConvScale(other->force_sdxl_vae_conv_scale);
    set_chromaUseDitMask(other->chroma_use_dit_mask);
    set_chromaUseT5Mask(other->chroma_t5_mask_pad);
    set_streamLayers(other->stream_layers);
    set_qwenImageZero(other->qwen_image_zero_cond_t);

     // m_ctxParams = other;
}



void QSdCtxParams::addEmbeddings(QString url, QString uid)
{
    QSdEmbedding *nItem = m_embeddings->getByUid(url);
    if(nItem == Q_NULLPTR){
        nItem = new QSdEmbedding{m_embeddings};
        nItem->set_embeddingName(uid);
        nItem->set_embeddingPath(url);
        m_embeddings->append(nItem);
    }
}

QString QSdCtxParams::debugString()
{
    QString ret = "Unknown";
    ret = QString::fromLatin1(sd_ctx_params_to_str(&m_ctxParams));
    return ret;
}

