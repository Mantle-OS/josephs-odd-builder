#include "qsdctx.h"

QSD::QSD(QObject *parent) :
    QObject{parent},
    m_ContextParams{new QSdCtxParams{this}},
    m_ImageGenerationParams{new QSdImgGenParams{this}},
    m_VideoGenerationParams{new QSdVidGenParams{this}},
    m_Backend{new ObjectListModel<QSdBackendDevice>{this, "", ""}}
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
    Q_EMIT totalStepsChanged();
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


    // thread pool collisions
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

    sd_img_gen_params_t params =
        m_ImageGenerationParams->imgGenParms();

    qDebug() << "sample_method =" << params.sample_params.sample_method
             << "scheduler =" << params.sample_params.scheduler
             << "steps =" << params.sample_params.sample_steps
             << "eta =" << params.sample_params.eta
             << "flow_shift =" << params.sample_params.flow_shift
             << "txt_cfg =" << params.sample_params.guidance.txt_cfg
             << "img_cfg =" << params.sample_params.guidance.img_cfg
             << "distilled =" << params.sample_params.guidance.distilled_guidance
             << "strength =" << params.strength
             << "width =" << params.width
             << "height =" << params.height
             << "seed =" << params.seed
             << "batch_count =" << params.batch_count;

    // sd_img_gen_params_t stableParamsSnapshot = m_ImageGenerationParams->imgGenParms();
    QFuture<SdGenerationResult> future = QtConcurrent::run(
        &QSD::runImageGenerationWorker,
        this,
        m_ctx,
        params,
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

    // Invoke the heavy "low-level" inference processing boundary
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
        const sd_image_t &raw = rawImagesArray[0];

        // qDebug() << "Native result:"
        //          << "width =" << raw.width
        //          << "height =" << raw.height
        //          << "channel =" << raw.channel
        //          << "data =" << static_cast<void *>(raw.data);

        if (raw.data && raw.width && raw.height && raw.channel) {
            const qsizetype byteCount =
                static_cast<qsizetype>(raw.width) *
                static_cast<qsizetype>(raw.height) *
                static_cast<qsizetype>(raw.channel);

            quint8 minimum = 255;
            quint8 maximum = 0;
            quint64 sum = 0;

            for (qsizetype i = 0; i < byteCount; ++i) {
                const quint8 value = raw.data[i];
                minimum = qMin(minimum, value);
                maximum = qMax(maximum, value);
                sum += value;
            }

            // qDebug() << "Native pixels:"
            //          << "bytes =" << byteCount
            //          << "min =" << minimum
            //          << "max =" << maximum
            //          << "average ="
            //          << static_cast<double>(sum) /
            //                 static_cast<double>(byteCount);
        }
        // -pixels onto target canvas object
        targetCanvas->setImg(rawImagesArray[0]);


        // qDebug() << "Copied target:"
        //          << "width =" << targetCanvas->img().width
        //          << "height =" << targetCanvas->img().height
        //          << "channel =" << targetCanvas->img().channel
        //          << "data =" << static_cast<void *>(targetCanvas->data());
        // Clean up
        free_sd_images(rawImagesArray, 1);

        // Save Processing Pass
        if (targetCanvas->data() != nullptr && snapshotResult.triggerAutoSave) {
            QString destinationDirectory = QString("%1/tmp").arg(QAiUtils::outDir);

            // save the file
            targetCanvas->saveToFile(destinationDirectory);
        }
    } else {
        qWarning() << "Asynchronous generation returned an invalid output buffer array.";
    }

    // Teardown context loop to cleanly free weights out of VRAM if running single-pass passes
    // SHOULD have a flag on this later on.
    if (m_ctx) {
        free_sd_ctx(m_ctx);
        m_ctx = nullptr;
    }
}