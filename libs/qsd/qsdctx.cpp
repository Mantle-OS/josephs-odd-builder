#include "qsdctx.h"

QSD::QSD(QObject *parent) :
    m_ContextParams{new QSdCtxParams{this}},
    m_ImageGenerationParams{new QSdImgGenParams{this}},
    m_VideoGenerationParams{new QSdVidGenParams{this}},
    m_Backend{new ObjectListModel<QSdBackendDevice>{this, "", ""}},
    QObject{parent}
{

    QAiUtils::debugPaths();

    setSystemInfo();
    setNumPhysicalCores();
    setSdVersion();
    setSdCommit();

    sd_set_progress_callback(progressionCallback, (void*)this);
    sd_set_log_callback(loggingCallback, (void*)this);

    auto s = m_ContextParams->ctxParams();
    m_ctx = new_sd_ctx(&s);
    if(m_ctx){
        qDebug() << "SD main backend done.";
        fillBackend();
    }else{
        qDebug() << "Failed to init SD main backend. Will try again later";
    }
    connect(&m_imageWatcher, &QFutureWatcher<sd_image_t*>::finished,
            this, &QSD::onGenerationFinished);

}

QSD::~QSD()
{
    if(!m_Backend->isEmpty()){
        m_Backend->clear();
    }

    if(m_Backend){
        delete m_Backend;
        m_Backend = nullptr;
    }


    if(m_ImageGenerationParams){
        delete m_ImageGenerationParams;
        m_ImageGenerationParams = nullptr;
    }

    if(m_VideoGenerationParams){
        delete m_VideoGenerationParams;
        m_VideoGenerationParams = nullptr;
    }


    if(m_ContextParams){
        delete m_ContextParams;
        m_ContextParams = nullptr;
    }

    if(m_ctx)
        free_sd_ctx(m_ctx);
}

qint32 QSD::numPhysicalCores() const
{
    return m_numPhysicalCores;
}

void QSD::setNumPhysicalCores()
{
    qint32 newNumPhysicalCores = sd_get_num_physical_cores();
    if (m_numPhysicalCores == newNumPhysicalCores) return;
    m_numPhysicalCores = newNumPhysicalCores;
    Q_EMIT numPhysicalCoresChanged();
}

QString QSD::systemInfo() const
{
    return m_systemInfo;
}

void QSD::setSystemInfo()
{
    QString  newSystemInfo = QString::fromLatin1(sd_get_system_info());
    if (m_systemInfo == newSystemInfo) return;
    m_systemInfo = newSystemInfo;
    Q_EMIT systemInfoChanged();
}

QString QSD::sdVersion() const
{
    return m_sdVersion;
}

void QSD::setSdVersion()
{
    QString  newSdVersion = QString::fromLatin1(sd_version());
    if (m_sdVersion == newSdVersion) return;
    m_sdVersion = newSdVersion;
    Q_EMIT sdVersionChanged();
}

QString QSD::sdCommit() const
{
    return m_sdCommit;
}

void QSD::setSdCommit()
{
    QString newSdCommit = QString::fromLatin1(sd_commit());
    if (m_sdCommit == newSdCommit) return;
    m_sdCommit = newSdCommit;
    Q_EMIT sdCommitChanged();
}

int QSD::currentStep() const
{
    return m_currentStep;
}

void QSD::setCurrentStep(int newCurrentStep)
{
    if (m_currentStep == newCurrentStep)
        return;
    m_currentStep = newCurrentStep;
    Q_EMIT currentStepChanged();
}

int QSD::totalSteps() const
{
    return m_totalSteps;
}

void QSD::setTotalSteps(int newTotalSteps)
{
    if (m_totalSteps == newTotalSteps)
        return;
    m_totalSteps = newTotalSteps;
    Q_EMIT totalSteps();
}

float QSD::progressionTime() const
{
    return m_progressionTime;
}

void QSD::setProgressionTime(float newProgressionTime)
{
    if (qFuzzyCompare(m_progressionTime, newProgressionTime)) return;
    m_progressionTime = newProgressionTime;
    Q_EMIT progressionTimeChanged();
}

bool QSD::supportsImageGen() const
{
    return sd_ctx_supports_image_generation(m_ctx);
}

bool QSD::supportsVideoGen() const
{
    return sd_ctx_supports_video_generation(m_ctx);
}

QSdEnums::QSdSampleTypes QSD::getDefaultSampleMethod()
{
    if(m_ctx)
        return QSdEnums::qsdSampleType( sd_get_default_sample_method(m_ctx) );
    return QSdEnums::QSdEulerA;
}

QSdEnums::QSdSchedulerTypes QSD::getDefaultScheduler(QSdEnums::QSdSampleTypes type)
{
    if(m_ctx)
        return QSdEnums::qsdSchedulerType(
            sd_get_default_scheduler(m_ctx, QSdEnums::sdSampleType(type))
            );

    return QSdEnums::QSdDiscrete;
}

void QSD::generateImage(QSdImage *outImage, bool autoSave)
{

    if (outImage)
        m_que.append(outImage);



    // 1. Guard check: Safety protection against thread pool collisions
    if (m_imageWatcher.isRunning()) {
        qWarning() << "Image generation engine is currently busy processing a frame!";
        return;
    }

    if (!outImage) {
        qWarning() << "Cannot generate image: Output target QSdImage pointer is null.";
        return;
    }

    if (m_ctx == nullptr) {
        fillBackend();
        auto ctxP = m_ContextParams->ctxParams();
        m_ctx = new_sd_ctx(&ctxP);
    }

    if (!m_ctx || !m_ImageGenerationParams)
        return;

    sd_img_gen_params_t stableParamsSnapshot = m_ImageGenerationParams->imgGenParms();
    QFuture<SdGenerationResult> future = QtConcurrent::run(
        &QSD::runImageGenerationWorker,
        this,
        m_ctx,
        stableParamsSnapshot,
        outImage,
        autoSave
        );

    m_imageWatcher.setFuture(future);
}

SdGenerationResult QSD::runImageGenerationWorker(sd_ctx_t* ctx,
                                                 sd_img_gen_params_t params,
                                                 QSdImage* target,
                                                 bool autoSave)
{
    SdGenerationResult outcomePacket;
    outcomePacket.targetImageElement = target;
    outcomePacket.triggerAutoSave = autoSave;

    // Invoke the heavy low-level inference processing boundary
    outcomePacket.resultImages = generate_image(ctx, &params);

    return outcomePacket;
}

void QSD::onGenerationFinished()
{
    // Extract the completed custom result configuration packet
    SdGenerationResult snapshotResult = m_imageWatcher.result();
    sd_image_t* rawImagesArray = snapshotResult.resultImages;
    QSdImage* targetCanvas = snapshotResult.targetImageElement;

    if (rawImagesArray && rawImagesArray->data != nullptr && targetCanvas != nullptr) {
        qInfo() << "Inference worker complete. Updating canvas texture data...";

        // 1. Map raw uncompressed C-pixels onto target canvas object
        targetCanvas->setImg(rawImagesArray[0]);

        // Clean up the temporary C memory allocation array returned by stable-diffusion.cpp
        free_sd_images(rawImagesArray, 1);

        // 2. Automated File-System Save Processing Pass
        // Note: checking !targetCanvas->data() or checking dimensions verifies it isn't empty
        if (targetCanvas->data() != nullptr && snapshotResult.triggerAutoSave) {
            QString destinationDirectory = QString("%1/tmp").arg(QAiUtils::outDir);

            // Your custom saveToFile logic safely handles suffix collisions automatically now!
            targetCanvas->saveToFile(destinationDirectory);
        }
    } else {
        qWarning() << "Asynchronous generation returned an invalid output buffer array.";
    }

    // Teardown context loop to cleanly free weights out of VRAM if running single-pass passes
    if (m_ctx) {
        free_sd_ctx(m_ctx);
        m_ctx = nullptr;
    }
}