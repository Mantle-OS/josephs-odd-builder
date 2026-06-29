#include "qaipathing.h"

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
    if(m_checkpointsDirs){
        delete m_checkpointsDirs;
        m_checkpointsDirs = nullptr;
    }
    if(m_availableCheckpoints){
        delete m_availableCheckpoints;
        m_availableCheckpoints = nullptr;
    }

    if(!m_diffusionDirs->isEmpty()){
        delete m_diffusionDirs;
        m_diffusionDirs = nullptr;
    }
    if(!m_availableDiffusionModels->isEmpty()){
        delete m_availableDiffusionModels;
        m_availableDiffusionModels = nullptr;
    }

    if(!m_textEncoderDirs->isEmpty()){
        delete m_textEncoderDirs;
        m_textEncoderDirs = nullptr;
    }
    if(!m_availableTextEncoders->isEmpty()){
        delete m_availableTextEncoders;
        m_availableTextEncoders = nullptr;
    }

    if(!m_lorasDirs->isEmpty()){
        delete m_lorasDirs;
        m_lorasDirs = nullptr;
    }
    if(!m_availableLoras->isEmpty()){
        delete m_availableLoras;
        m_availableLoras = nullptr;
    }

    if(!m_embeddingsDirs->isEmpty()){
        delete m_embeddingsDirs;
        m_embeddingsDirs = nullptr;
    }
    if(!m_availableEmbeddings->isEmpty()){
        delete m_availableEmbeddings;
        m_availableEmbeddings = nullptr;
    }

    if(!m_controlNetDirs->isEmpty()){
        delete m_controlNetDirs;
        m_controlNetDirs = nullptr;
    }
    if(!m_availableControlNet->isEmpty()){
        delete m_availableControlNet;
        m_availableControlNet = nullptr;
    }

    if(!m_upscaleModelDirs->isEmpty()){
        delete m_upscaleModelDirs;
        m_upscaleModelDirs = nullptr;
    }
    if(!m_availableUpscaleModels->isEmpty()){
        delete m_availableUpscaleModels;
        m_availableUpscaleModels = nullptr;
    }

    if(!m_vaeModelDirs->isEmpty()){
        delete m_vaeModelDirs;
        m_vaeModelDirs = nullptr;
    }
    if(!m_availableVaeModels->isEmpty()){
        delete m_availableVaeModels;
        m_availableVaeModels = nullptr;
    }

    if(!m_audioVaeDirs->isEmpty()){
        delete m_audioVaeDirs;
        m_audioVaeDirs = nullptr;
    }
    if(!m_availableAudioVae->isEmpty()){
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

void QAiPathing::scan(QmlStringList *dirs, QmlStringList *files,
          const QStringList &allowedSuffix)
{
    for (int i=0 ; i <= dirs->size(); i++){
        const QString &dir = dirs->at(i);
        if(QAiUtils::dirExists(dir)){
            QDirIterator dit(dir,
                             QDir::Files| QDir::NoDotAndDotDot | QDir::NoDot,
                             QDirIterator::Subdirectories);
            while(dit.hasNext()){
                QString file(dit.next());
                QFileInfo fInfo(file);

                if(allowedSuffix.contains(fInfo.suffix())){
                    if(!file.contains(file)){
                        files->append(file);
                    }
                }
            }
        }
    }
}

