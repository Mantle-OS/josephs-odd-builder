#include "qllamalocalengine.h"
#include <QDebug>

void QLlamaLocalWorker::process()
{
    m_abort.store(false);

    if (!m_model || !m_context || !m_sampler) {
        Q_EMIT errorOccurred("Missing internal hardware compute layers.");
        return;
    }

    const struct llama_vocab* nativeVocab = m_model->nativeVocab();
    if (!nativeVocab) {
        Q_EMIT errorOccurred("Failed to acquire target model vocabulary tracking layer.");
        return;
    }

    QByteArray promptBytes = m_prompt.toUtf8();
    std::vector<llama_token> tokens(promptBytes.size() + 4);
    int32_t n_tokens = llama_tokenize(nativeVocab, promptBytes.constData(), promptBytes.size(),
                                      tokens.data(), tokens.size(), true, true);
    if (n_tokens < 0) {
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(nativeVocab,
                                  promptBytes.constData(),
                                  promptBytes.size(),
                                  tokens.data(),
                                  tokens.size(),
                                  true,
                                  true);
    }
    tokens.resize(n_tokens);

    QList<int> currentInput;
    for (int32_t i = 0; i < n_tokens; ++i)
        currentInput.append(static_cast<int>(tokens[i]));


    QString accumulatedResult = "";
    int32_t maxTokens = llama_n_ctx(m_context->nativeContext());
    if (maxTokens <= 0)
        maxTokens = 2048; // Safe fallback barrier ?>?????? Look up this later JOSEPH TODO

    while (m_context->get_kvTokensUsed() < maxTokens) {
        if (m_abort.load()) {
            break;
        }

        // Cross the ABI boundary using our pre-compiled sampler parameters ?
        int predictedToken = m_context->sampleNextToken(m_sampler, currentInput);
        if (predictedToken < 0) {
            Q_EMIT errorOccurred("Inference processing pipeline failure tracking inside hardware layer.");
            return;
        }

        // Check if we hit an end-of-generation barrier (EOS/EOT)
        if (llama_vocab_is_eog(nativeVocab, static_cast<llama_token>(predictedToken)))
            break;

        // Translate token element back to plaintext piece string
        char pieceBuf[256];
        int32_t pieceLen = llama_token_to_piece(nativeVocab,
                                                static_cast<llama_token>(predictedToken),
                                                pieceBuf,
                                                sizeof(pieceBuf),
                                                0,
                                                true);
        if (pieceLen > 0) {
            QString chunk = QString::fromUtf8(pieceBuf, pieceLen);
            accumulatedResult.append(chunk);
            Q_EMIT tokenGenerated(chunk);
        }

        // Cycle prediction targets -->
        currentInput.clear();
        currentInput.append(predictedToken);
    }

    Q_EMIT finished(accumulatedResult);
}

QLlamaLocalEngine::QLlamaLocalEngine(QObject *parent)
    : QLlamaEngine{parent}
{
}

QLlamaLocalEngine::~QLlamaLocalEngine()
{
    cancel(); // virtual fixup later for warnings TODO JOSEPH
}

void QLlamaLocalEngine::generate()
{
    if (get_isProcessing())
        return;

    if (!get_model() || !get_context() || !get_sampler()) {
        Q_EMIT executionError("Compute pipeline wrappers are unallocated or unlinked.");
        return;
    }

    set_isProcessing(true);
    set_streamingText("");
    Q_EMIT generationStarted();

    // Spawn and configure background execution assets dynamically
    m_workerThread = new QThread();
    m_worker = new QLlamaLocalWorker();
    m_worker->setParams(get_model(), get_context(), get_sampler(), get_prompt());
    m_worker->moveToThread(m_workerThread);

    // Cross-thread signals
    connect(m_workerThread, &QThread::started, m_worker, &QLlamaLocalWorker::process);

    connect(m_worker, &QLlamaLocalWorker::tokenGenerated, this, [this](const QString &chunk) {
        set_streamingText(get_streamingText() + chunk);
        Q_EMIT tokenStreamed(chunk);
    }, Qt::QueuedConnection);

    connect(m_worker, &QLlamaLocalWorker::finished, this, [this](const QString &fullText) {
        cancel(); // Clean up thread lifecycle handles safely
        Q_EMIT generationFinished(fullText);
    }, Qt::QueuedConnection);

    connect(m_worker, &QLlamaLocalWorker::errorOccurred, this, [this](const QString &err) {
        cancel();
        Q_EMIT executionError(err);
    }, Qt::QueuedConnection);

    m_workerThread->start();
}

void QLlamaLocalEngine::cancel()
{
    if (m_worker) {
        m_worker->requestAbort();
    }

    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
        m_workerThread = nullptr;
    }

    if (m_worker) {
        delete m_worker;
        m_worker = nullptr;
    }

    if (get_isProcessing()) {
        set_isProcessing(false);
    }
}
