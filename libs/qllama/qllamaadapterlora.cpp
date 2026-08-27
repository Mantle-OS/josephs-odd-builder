#include "qllamaadapterlora.h"
#include <QDebug>
#include <QFileInfo>

QLlamaAdapterLora::QLlamaAdapterLora(QObject *parent)
    : QLlamaBase(parent)
{
}

QLlamaAdapterLora::~QLlamaAdapterLora()
{
    unloadAdapter();
}

bool QLlamaAdapterLora::loadAdapter(QLlamaModel *model)
{
    if (m_isLoaded) {
        unloadAdapter();
    }

    if (!model || !model->get_isLoaded()) {
        set_lastErrorString("Cannot load a LoRA adapter: target base model is missing or unallocated.");
        return false;
    }

    if (m_loraPath.isEmpty()) {
        set_lastErrorString("LoRA adapter filepath target is empty.");
        return false;
    }

    QFileInfo checkFile(m_loraPath);
    if (!checkFile.exists() || !checkFile.isFile()) {
        set_lastErrorString(QString("LoRA adapter binary asset not found at: %1").arg(m_loraPath));
        return false;
    }

    // qDebug() << "[qllama] Parsing LoRA adapter weights matrix layer from:" << m_loraPath;

    // Cross the ABI barrier to initialize and map the fine-tuning adapter fields
    m_adapter = llama_adapter_lora_init(model->nativeModel(), m_loraPath.toUtf8().constData());

    if (!m_adapter) {
        set_lastErrorString("llama_adapter_lora_init failed. Ensure token dim or rank matches base model structural layout.");
        return false;
    }

    set_isLoaded(true);
    return true;
}

void QLlamaAdapterLora::unloadAdapter()
{
    if (m_adapter) {
        // qDebug() << "[qllama] Freeing native LoRA structural allocations.";
        // Releases the fine-tuning tracking resources safely from the heap
        llama_adapter_lora_free(m_adapter);
        m_adapter = nullptr;
    }
    set_isLoaded(false);
}
