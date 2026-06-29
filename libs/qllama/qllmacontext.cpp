#include "qllmacontext.h"
#include <QDebug>

QLlamaContext::QLlamaContext(QObject *parent) :
    QLlamaBase{parent},
    m_loraModel{new ObjectListModel<QLlamaAdapterLora>{this, "loraPath", "loraPath"}},
    m_batch{}
{
}

QLlamaContext::~QLlamaContext()
{
    releaseContext();
}

bool QLlamaContext::initContext(QLlamaModel *model, QLlamaContextParams *params)
{
    if (m_context) {
        releaseContext();
    }

    if (!model || !model->get_isLoaded()) {
        set_lastErrorString("Cannot initialize execution context: Base model weights are missing or unallocated.");
        return false;
    }

    if (!params) {
        set_lastErrorString("Cannot initialize context: Valid parameters wrapper is null.");
        return false;
    }

    m_linkedModel = model;
    llama_context_params nativeCtxParams = params->prepareContextParams();

    qDebug() << "[qllama] Provisioning compute sub-graph memory contexts...";
    // TODO this is "deprecated" next time stable diffision ups the ggml then we will update.
    m_context = llama_new_context_with_model(m_linkedModel->nativeModel(), nativeCtxParams);

    if (!m_context) {
        set_lastErrorString("llama_new_context_with_model failed to assign VRAM/RAM compute arenas.");
        return false;
    }

    m_batch = llama_batch_init(static_cast<int32_t>(params->get_nBatch()), 0, 1);
    m_batchAllocated = true;

    set_isActive(true);
    set_kvTokensUsed(0);
    Q_EMIT contextCreated();

    syncActiveLoRAs();

    return true;
}

void QLlamaContext::releaseContext()
{
    if (m_batchAllocated) {
        llama_batch_free(m_batch);
        m_batchAllocated = false;
    }

    if (m_context) {
        qDebug() << "[qllama] Tearing down active compute graph runtime layers.";
        llama_free(m_context);
        m_context = nullptr;
    }

    if (!m_loraModel->isEmpty()) {
        m_loraModel->clear(); // Drop tracking references safely via your template model
    }

    m_linkedModel = nullptr;

    if (m_isActive) {
        set_isActive(false);
        set_kvTokensUsed(0);
        Q_EMIT contextDestroyed();
    }
}


void QLlamaContext::clearKvCache()
{
    if (m_context) {
        qDebug() << "[qllama] Clearing context execution memory state via public C-API...";
        llama_memory_t mem = ::llama_get_memory(m_context);

        if (mem) {
            ::llama_memory_clear(mem, true);

            qDebug() << "[qllama] Success. Public memory blocks cleared.";
        } else {
            qWarning() << "[qllama] Failed to clear: Context returned a null memory handle pointer.";
        }

        // 3. Reset your internal framework primitive counter
        set_kvTokensUsed(0);
        Q_EMIT kvCacheReset();
    }
}
bool QLlamaContext::syncActiveLoRAs()
{
    if (!m_context) return false;

    clearKvCache();
    set_kvTokensUsed(0);

    if (m_loraModel->isEmpty()) {
        ::llama_set_adapters_lora(m_context, nullptr, 0, nullptr);
        return true;
    }

    std::vector<struct llama_adapter_lora*> nativeAdapters;
    std::vector<float>                      nativeScales;

    nativeAdapters.reserve(m_loraModel->size());
    nativeScales.reserve(m_loraModel->size());

    for (int i = 0; i < m_loraModel->size(); ++i) {
        auto *adapterObj = qobject_cast<QLlamaAdapterLora*>(m_loraModel->get(i));
        if (adapterObj && adapterObj->get_isLoaded()) {
            nativeAdapters.push_back(adapterObj->nativeAdapter());
            nativeScales.push_back(static_cast<float>(adapterObj->get_scale()));
        }
    }

    if (!nativeAdapters.empty()) {
        qDebug() << "[qllama] Syncing" << nativeAdapters.size() << "active LoRA adapters over to the graph execution core.";

        // Use the public API call to cross the C/C++ ABI boundary cleanly
        ::llama_set_adapters_lora(
            m_context,
            nativeAdapters.data(),
            nativeAdapters.size(),
            nativeScales.data()
            );
    } else {
        ::llama_set_adapters_lora(m_context, nullptr, 0, nullptr);
    }

    return true;
}

int QLlamaContext::sampleNextToken(QLlamaSampler *sampler, const QList<int> &inputTokens)
{
    if (!m_context || !sampler || inputTokens.isEmpty())
        return -1;

    sampler->setVocabContextSize(llama_vocab_n_tokens(llama_model_get_vocab(m_linkedModel->nativeModel())));
    struct llama_sampler* nativeChain = sampler->prepareSamplerChain();

    if (!nativeChain) {
        set_lastErrorString("Inference Aborted: Sampler chain generation failed to return a valid processing node graph.");
        return -1;
    }

    m_batch.n_tokens = 0;

    for (int i = 0; i < inputTokens.size(); ++i) {
        m_batch.token[m_batch.n_tokens]     = static_cast<llama_token>(inputTokens[i]);
        m_batch.pos[m_batch.n_tokens]       = m_kvTokensUsed + m_batch.n_tokens;
        m_batch.n_seq_id[m_batch.n_tokens]  = 1;
        m_batch.seq_id[m_batch.n_tokens][0] = 0;
        m_batch.logits[m_batch.n_tokens]    = (i == inputTokens.size() - 1);

        m_batch.n_tokens++;
    }

    int decodeStatus = llama_decode(m_context, m_batch);
    if (decodeStatus != 0) {
        set_lastErrorString(QString("llama_decode execution failure encountered inside hardware pipelines. Code: %1").arg(decodeStatus));
        return -1;
    }

    m_kvTokensUsed += m_batch.n_tokens;
    set_kvTokensUsed(m_kvTokensUsed);

    llama_token predictedToken = llama_sampler_sample(nativeChain, m_context, -1);

    return static_cast<int>(predictedToken);
}
