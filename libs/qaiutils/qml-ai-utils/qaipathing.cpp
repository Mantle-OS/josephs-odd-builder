#include "qaipathing.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

QAiPathing::QAiPathing(QObject *parent) :
    QObject{parent},
    m_checkpointsDirs{new QmlStringList{this}},
    m_availableCheckpoints{new QmlStringList{this}},
    m_diffusionDirs{new QmlStringList{this}},
    m_availableDiffusionModels{new QmlStringList{this}},
    m_textEncoderDirs{new QmlStringList{this}},
    m_availableTextEncoders{new QmlStringList{this}},
    m_lorasDirs{new QmlStringList{this}},
    m_availableLoras{new QmlStringList{this}},
    m_embeddingsDirs{new QmlStringList{this}},
    m_availableEmbeddings{new QmlStringList{this}},
    m_controlNetDirs{new QmlStringList{this}},
    m_availableControlNet{new QmlStringList{this}},
    m_upscaleModelDirs{new QmlStringList{this}},
    m_availableUpscaleModels{new QmlStringList{this}},
    m_vaeModelDirs{new QmlStringList{this}},
    m_availableVaeModels{new QmlStringList{this}},
    m_audioVaeDirs{new QmlStringList{this}},
    m_availableAudioVae{new QmlStringList{this}}
{
    scanAll();
}


QAiPathing::~QAiPathing()
{
    if (m_checkpointsDirs) {
        if (!m_checkpointsDirs->isEmpty())
            m_checkpointsDirs->clear();
        delete m_checkpointsDirs;
        m_checkpointsDirs = nullptr;
    }

    if (m_availableCheckpoints) {
        if (!m_availableCheckpoints->isEmpty())
            m_availableCheckpoints->clear();
        delete m_availableCheckpoints;
        m_availableCheckpoints = nullptr;
    }

    if (m_diffusionDirs) {
        if (!m_diffusionDirs->isEmpty())
            m_diffusionDirs->clear();
        delete m_diffusionDirs;
        m_diffusionDirs = nullptr;
    }

    if (m_availableDiffusionModels) {
        if (!m_availableDiffusionModels->isEmpty())
            m_availableDiffusionModels->clear();
        delete m_availableDiffusionModels;
        m_availableDiffusionModels = nullptr;
    }

    if (m_textEncoderDirs) {
        if (!m_textEncoderDirs->isEmpty())
            m_textEncoderDirs->clear();
        delete m_textEncoderDirs;
        m_textEncoderDirs = nullptr;
    }

    if (m_availableTextEncoders) {
        if (!m_availableTextEncoders->isEmpty())
            m_availableTextEncoders->clear();
        delete m_availableTextEncoders;
        m_availableTextEncoders = nullptr;
    }

    if (m_lorasDirs) {
        if (!m_lorasDirs->isEmpty())
            m_lorasDirs->clear();
        delete m_lorasDirs;
        m_lorasDirs = nullptr;
    }

    if (m_availableLoras) {
        if (!m_availableLoras->isEmpty())
            m_availableLoras->clear();
        delete m_availableLoras;
        m_availableLoras = nullptr;
    }

    if (m_embeddingsDirs) {
        if (!m_embeddingsDirs->isEmpty())
            m_embeddingsDirs->clear();
        delete m_embeddingsDirs;
        m_embeddingsDirs = nullptr;
    }

    if (m_availableEmbeddings) {
        if (!m_availableEmbeddings->isEmpty())
            m_availableEmbeddings->clear();
        delete m_availableEmbeddings;
        m_availableEmbeddings = nullptr;
    }

    if (m_controlNetDirs) {
        if (!m_controlNetDirs->isEmpty())
            m_controlNetDirs->clear();
        delete m_controlNetDirs;
        m_controlNetDirs = nullptr;
    }

    if (m_availableControlNet) {
        if (!m_availableControlNet->isEmpty())
            m_availableControlNet->clear();
        delete m_availableControlNet;
        m_availableControlNet = nullptr;
    }

    if (m_upscaleModelDirs) {
        if (!m_upscaleModelDirs->isEmpty())
            m_upscaleModelDirs->clear();
        delete m_upscaleModelDirs;
        m_upscaleModelDirs = nullptr;
    }

    if (m_availableUpscaleModels) {
        if (!m_availableUpscaleModels->isEmpty())
            m_availableUpscaleModels->clear();
        delete m_availableUpscaleModels;
        m_availableUpscaleModels = nullptr;
    }

    if (m_vaeModelDirs) {
        if (!m_vaeModelDirs->isEmpty())
            m_vaeModelDirs->clear();
        delete m_vaeModelDirs;
        m_vaeModelDirs = nullptr;
    }

    if (m_availableVaeModels) {
        if (!m_availableVaeModels->isEmpty())
            m_availableVaeModels->clear();
        delete m_availableVaeModels;
        m_availableVaeModels = nullptr;
    }

    if (m_audioVaeDirs) {
        if (!m_audioVaeDirs->isEmpty())
            m_audioVaeDirs->clear();
        delete m_audioVaeDirs;
        m_audioVaeDirs = nullptr;
    }

    if (m_availableAudioVae) {
        if (!m_availableAudioVae->isEmpty())
            m_availableAudioVae->clear();
        delete m_availableAudioVae;
        m_availableAudioVae = nullptr;
    }
}


void QAiPathing::scanAll(){
    scanCheckpoints();
    scanDiffusionModels();
    scanTextEncoders();
    scanLoras();
    scanEmbeddings();
    scanControlNet();
    scanUpscaleModel();
    scanVaeModel();
    scanAudioVaeModel();
    Q_EMIT scanDone();
}

void QAiPathing::scanCheckpoints()
{
    if(m_checkpointsDirs->isEmpty())
        m_checkpointsDirs->append(QAiUtils::checkpointsDir);

    scan(m_checkpointsDirs, m_availableCheckpoints);
    Q_EMIT scanCheckpointsDone();
}

void QAiPathing::scanDiffusionModels()
{
    if(m_diffusionDirs->isEmpty())
        m_diffusionDirs->append(QAiUtils::diffusionDir);

    scan(m_diffusionDirs, m_availableDiffusionModels);
    Q_EMIT scanDiffusionDone();
}

void QAiPathing::scanTextEncoders()
{
    if(m_textEncoderDirs->isEmpty())
        m_textEncoderDirs->append(QAiUtils::textEncoderDir);

    scan(m_textEncoderDirs, m_availableTextEncoders);
    Q_EMIT scanTextEncodersDone();
}

void QAiPathing::scanLoras()
{
    if(m_lorasDirs->isEmpty())
        m_lorasDirs->append(QAiUtils::lorasDir);

    scan(m_lorasDirs, m_availableLoras);
    Q_EMIT scanLorasDone();
}

void QAiPathing::scanEmbeddings()
{
    if(m_embeddingsDirs->isEmpty())
        m_embeddingsDirs->append(QAiUtils::embeddingsDir);

    scan(m_embeddingsDirs, m_availableEmbeddings);
    Q_EMIT scanEmbeddingsDone();
}

void QAiPathing::scanControlNet()
{
    if(m_controlNetDirs->isEmpty())
        m_controlNetDirs->append(QAiUtils::controlNetDir);

    scan(m_controlNetDirs, m_availableControlNet);
    Q_EMIT scanControlNetDone();
}

void QAiPathing::scanUpscaleModel()
{
    if(m_upscaleModelDirs->isEmpty())
        m_upscaleModelDirs->append(QAiUtils::upscaleModelDir);

    scan(m_upscaleModelDirs, m_availableUpscaleModels);
    Q_EMIT scanUpscaleModelDone();
}

void QAiPathing::scanVaeModel()
{
    if(m_vaeModelDirs->isEmpty())
        m_vaeModelDirs->append(QAiUtils::vaeDir);

    scan(m_vaeModelDirs, m_availableVaeModels);
    Q_EMIT scanVaeModelDone();
}

void QAiPathing::scanAudioVaeModel()
{
    if(m_audioVaeDirs->isEmpty())
        m_audioVaeDirs->append(QAiUtils::audioVaeDir);

    scan(m_audioVaeDirs, m_availableAudioVae);
    Q_EMIT scanAudioVaeDone();
}

void QAiPathing::scan(QmlStringList *dirs, QmlStringList *files, const QStringList &allowedSuffix)
{
    if (!dirs || !files)
        return;

    for (int i = 0; i < dirs->size(); ++i) {
        QString const dir = dirs->at(i);

        if (!QAiUtils::dirExists(dir))
            continue;

        QDirIterator dit{
            dir,
            QDir::Files | QDir::NoDotAndDotDot,
            QDirIterator::Subdirectories
        };

        while (dit.hasNext()) {
            QString const file = dit.next();
            QFileInfo const fileInfo{file};

            if (!allowedSuffix.contains(fileInfo.suffix(), Qt::CaseInsensitive))
                continue;

            if (!files->contains(file))
                files->append(file);
        }
    }
}
