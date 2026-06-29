#include "qllamamodel.h"
#include <QDebug>
#include <QFileInfo>

QLlamaModel::QLlamaModel(QObject *parent) :
    QLlamaBase{parent}
{
}

QLlamaModel::~QLlamaModel()
{
    unloadModel();
}

bool QLlamaModel::loadModel(QLlamaModelParams *params)
{
    /// this will need some help
    if (m_isLoaded)
        unloadModel();

    if (m_modelPath.isEmpty()) {
        set_lastErrorString("Model path is completely empty.");
        return false;
    }

    QFileInfo fileInfo(m_modelPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        set_lastErrorString(QString("GGUF target asset file not found at: %1").arg(m_modelPath));
        return false;
    }

    llama_model_params nativeParams = params ? params->prepareParams() : llama_model_default_params();

    qDebug() << "[qllama] Initializing core model weights allocation pass from:" << m_modelPath;

    const QByteArray pathBatt = m_modelPath.toUtf8();
    m_model = llama_model_load_from_file(pathBatt.constData(), nativeParams);

    if (!m_model) {
        set_lastErrorString(QString("llama_model_load_from_file failed to process target weights file. Check VRAM limits."));
        return false;
    }

    m_vocab = llama_model_get_vocab(m_model);

    set_parameterCount(::llama_model_n_params(m_model));
    set_tensorCount(static_cast<quint64>(::llama_model_size(m_model))); // Uses absolute storage weight footprint

    char descBuffer[256] = {0};
    int32_t descLen = ::llama_model_desc(m_model, descBuffer, sizeof(descBuffer) - 1);
    if (descLen > 0)
        set_architecture(QString::fromUtf8(descBuffer));
    else
        set_architecture("unknown");

    set_isLoaded(true);
    Q_EMIT modelLoaded();

    qDebug() << "[qllama] Success. Loaded" << get_architecture() << "architecture engine with size"
             << get_tensorCount() << "bytes and" << get_parameterCount() << "active parameters.";

    return true;
}
void QLlamaModel::unloadModel()
{
    if (m_model) {
        qDebug() << "[qllama] Releasing native model allocations from system heap.";
        llama_model_free(m_model);
        m_model = nullptr;
        m_vocab = nullptr; // Dropped automatically since it's owned by the parent model handle
    }

    if (m_isLoaded) {
        set_isLoaded(false);
        set_tensorCount(0);
        set_parameterCount(0);
        set_architecture("unknown");
        Q_EMIT modelUnloaded();
    }
}

