#ifndef QSDCTX_H
#define QSDCTX_H

#include <QObject>
#include <QQmlEngine>
#include <QDebug>
#include <QQueue>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

#include <stable-diffusion.h>

#include <objectmodel.h>

#include "qsdbackenddevice.h"
#include "qsdctxparams.h"
#include "qsdimage.h"
#include "qsdimggenparams.h"
#include "qsdvidgenparams.h"

struct SdGenerationResult {
    sd_image_t* resultImages = nullptr;
    QSdImage* targetImageElement = nullptr;
    bool triggerAutoSave = false;
};

class QSD : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint32   numPhysicalCores    READ numPhysicalCores   NOTIFY numPhysicalCoresChanged  FINAL   )
    Q_PROPERTY(QString  systemInfo          READ systemInfo         NOTIFY systemInfoChanged        FINAL   )
    Q_PROPERTY(QString  sdVersion           READ sdVersion          NOTIFY sdVersionChanged         FINAL   )
    Q_PROPERTY(QString  sdCommit            READ sdCommit           NOTIFY sdCommitChanged          FINAL   )

    QP_RO(QString,      lastLog,            ""                                                              )
    QP_RW(bool,         logToConsole,       true                                                            )
    QP_RW(int,          logScrollback,      -1                                                              )

    // Progressions
    Q_PROPERTY(int      currentStep         READ currentStep        NOTIFY currentStepChanged       FINAL   )
    Q_PROPERTY(int      totalSteps          READ totalSteps         NOTIFY totalStepsChanged        FINAL   )
    Q_PROPERTY(float    progressionTime     READ progressionTime    NOTIFY progressionTimeChanged   FINAL   )
    // Params
    QP_PTR_RO(QSdCtxParams,                      ContextParams                                              )
    QP_PTR_RO(QSdImgGenParams,                   ImageGenerationParams                                      )
    QP_PTR_RO(QSdVidGenParams,                   VideoGenerationParams                                      )
    QP_PTR_RO(ObjectListModel<QSdBackendDevice>, Backend                                                    )


    QML_ELEMENT
    QML_SINGLETON

public:
    explicit QSD(QObject *parent = nullptr);
    ~QSD();

    static void progressionCallback(int step, int steps, float time, void *data)
    {
        QSD *ctx = static_cast<QSD *>(data);
        if (!ctx)
            return;
        ctx->setCurrentStep(step);
        ctx->setTotalSteps(steps);
        ctx->setProgressionTime(time);
    }


    static void loggingCallback(enum sd_log_level_t level, const char *text, void* data)
    {


        QSD *ctx = static_cast<QSD *>(data);
        if (!ctx)
            return;

        auto log = QString::fromLatin1(text).simplified();
        switch (level) {
        case SD_LOG_DEBUG:
            ctx->set_lastLog(log);
            break;
        case SD_LOG_INFO:
            ctx->set_lastLog(log);
            break;
        case SD_LOG_WARN:
            ctx->set_lastLog(log);
            break;
        case SD_LOG_ERROR:
            ctx->set_lastLog(log);
            qErrnoWarning(text);
            break;
        }



        qDebug() << log;

    }


    qint32 numPhysicalCores() const;
    QString systemInfo() const;
    QString sdVersion() const;
    QString sdCommit() const;

    int currentStep() const;
    void setCurrentStep(int newCurrentStep);

    int totalSteps() const;
    void setTotalSteps(int newTotalSteps);

    float progressionTime() const;
    void setProgressionTime(float newProgressionTime);

    Q_INVOKABLE bool supportsImageGen() const;
    Q_INVOKABLE bool supportsVideoGen() const;

    Q_INVOKABLE QSdEnums::QSdSampleTypes getDefaultSampleMethod();
    Q_INVOKABLE QSdEnums::QSdSchedulerTypes getDefaultScheduler(QSdEnums::QSdSampleTypes type);

    Q_INVOKABLE void generateImage(QSdImage *outImage = nullptr, bool autoSave = false);


    bool convertWeights(const QString &inputPath, const QString &vaePath, const QString &outputPath,
                        QSdEnums::QSdWeightTypes outputType, const QString &tensorTypeRules, bool convertName )
    {
        return convert(inputPath.toLatin1().constData(),
                       vaePath.toLatin1().constData(),
                       outputPath.toLatin1().constData(),
                       QSdEnums::sdWeightType(outputType),
                       tensorTypeRules.toLatin1().constData(),
                       convertName);
    }

    bool preprocessCanny(QSdImage image, float highThreshold, float lowThreshold,
                         float weak, float strong, bool inverse)
    {
        return preprocess_canny(image.img(),
                                highThreshold,
                                lowThreshold,
                                weak,
                                strong,
                                inverse);
    }

    // SD_API bool generate_video(sd_ctx_t* sd_ctx,
    //                            const sd_vid_gen_params_t* sd_vid_gen_params,
    //                            sd_image_t** frames_out,
    //                            int* num_frames_out,
    //                            sd_audio_t** audio_out);
    // IMPLY BACKLOG AFTER IMAGE GEN
    // bool generateVideo(){}

    ////////////////////////////////////////////
    // START UPSCALLER BACKLOG
    ////////////////////////////////////////////
    // Impl:



    // SD_API upscaler_ctx_t* new_upscaler_ctx(const char* esrgan_path,
    //                                         bool direct,
    //                                         int n_threads,
    //                                         int tile_size,
    //                                         const char* backend,
    //                                         const char* params_backend);

    // Impl:
    // SD_API void free_upscaler_ctx(upscaler_ctx_t* upscaler_ctx);

    // Impl:
    // SD_API sd_image_t upscale(upscaler_ctx_t* upscaler_ctx,
    //                           sd_image_t input_image,
    //                           uint32_t upscale_factor);
    // void upscaleImage(const QString &esrganPath, bool direct, int n_threads, int tileSize,
    //                   const QString &backend, const QString &paramsBackend)
    // {

    // }

    // Impl:
    // SD_API int get_upscale_factor(upscaler_ctx_t* upscaler_ctx);


    ////////////////////////////////////////////
    // END UPSCALLER
    ////////////////////////////////////////////





    // Impl:
    // SD_API void free_sd_images(sd_image_t* result_images, int num_images);

private Q_SLOTS:
    void onGenerationFinished();

Q_SIGNALS:
    void numPhysicalCoresChanged();
    void systemInfoChanged();
    void sdVersionChanged();
    void sdCommitChanged();

    void currentStepChanged();
    void totalStepsChanged();
    void progressionTimeChanged();

private:
    void setNumPhysicalCores();
    void setSystemInfo();
    void setSdVersion();
    void setSdCommit();


    qint32  m_numPhysicalCores = 0;
    QString m_systemInfo;
    QString m_sdVersion;
    QString m_sdCommit;

    sd_ctx_t *m_ctx = nullptr;
    int m_currentStep;
    int m_totalSteps;
    float m_progressionTime;

    QQueue<QSdImage*> m_que;


    QFutureWatcher<SdGenerationResult> m_imageWatcher;
    SdGenerationResult runImageGenerationWorker(sd_ctx_t* ctx,
                                                sd_img_gen_params_t params,
                                                QSdImage* target,
                                                bool autoSave);
    void fillBackend(){
        if(m_ctx){
            // Now we can fill the backend manager
            if(!m_Backend->isEmpty())
                m_Backend->clear();

            for (int i = 0; i <= static_cast<int>(QSdEnums::Upscaler); ++i) {
                auto *dev = new QSdBackendDevice(this);
                dev->set_moduleType(i);
                if (i == QSdEnums::TextEncoder || i == QSdEnums::ClipVision) {
                    dev->set_runtimeTarget("cpu"); // Save valuable VRAM by defaulting encoders to host memory!
                    dev->set_paramsTarget("cpu");
                }
                m_Backend->append(dev);
            }
        }
    }

};

#endif // QSDCTX_H
//sd_get_num_physical_cores()