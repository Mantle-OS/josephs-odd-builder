#ifndef QAIPATHING_H
#define QAIPATHING_H

#include <QObject>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <QQmlEngine>

#include "qmlstringlist.h"
#include "qaiutils.h"

#include "property-macros.h"
#include "pointer-macros.h"


class QAiPathing : public QObject
{
    Q_OBJECT
    QP_RW(QString,              baseDir,                QAiUtils::modelsDir         )

    QP_RW(QString,              checkpoint,             ""                          ) // The current file
    QP_PTR_RO(QmlStringList,    checkpointsDirs                                     ) // The dirs that can contain the files
    QP_PTR_RO(QmlStringList,    availableCheckpoints                                ) // a list of all the files in the dirs that match the suffixs

    QP_RW(QString,              diffusion,              ""                          )
    QP_PTR_RO(QmlStringList,    diffusionDirs                                       )
    QP_PTR_RO(QmlStringList,    availableDiffusionModels                            )

    QP_RW(QString,              textEncoder,            ""                          )
    QP_PTR_RO(QmlStringList,    textEncoderDirs                                     )
    QP_PTR_RO(QmlStringList,    availableTextEncoders                               )

    QP_RW(QString,              lorasDir,               QAiUtils::lorasDir          )
    QP_PTR_RO(QmlStringList,    lorasDirs                                           )
    QP_PTR_RO(QmlStringList,    availableLoras                                      )

    QP_RW(QString,              embeddingsDir,          QAiUtils::embeddingsDir     )
    QP_PTR_RO(QmlStringList,    embeddingsDirs                                      )
    QP_PTR_RO(QmlStringList,    availableEmbeddings                                 )

    QP_RW(QString,              controlNet,             ""                          )
    QP_PTR_RO(QmlStringList,    controlNetDirs                                      )
    QP_PTR_RO(QmlStringList,    availableControlNet                                 )

    QP_RW(QString,              upscaleModel,           ""                          )
    QP_PTR_RO(QmlStringList,    upscaleModelDirs                                    )
    QP_PTR_RO(QmlStringList,    availableUpscaleModels                              )

    QP_RW(QString,              vaeModel,               ""                          )
    QP_PTR_RO(QmlStringList,    vaeModelDirs                                        )
    QP_PTR_RO(QmlStringList,    availableVaeModels                                  )

    QP_RW(QString,              audioVae,               ""                          )
    QP_PTR_RO(QmlStringList,    audioVaeDirs                                        )
    QP_PTR_RO(QmlStringList,    availableAudioVae                                   )


    QML_ELEMENT
    QML_SINGLETON
public:
    explicit QAiPathing(QObject *parent = nullptr);
    ~QAiPathing();
    Q_INVOKABLE void scanAll();



Q_SIGNALS:
    void scanCheckpointsDone();
    void scanDiffusionDone();
    void scanTextEncodersDone();
    void scanLorasDone();
    void scanEmbeddingsDone();
    void scanControlNetDone();
    void scanUpscaleModelDone();
    void scanVaeModelDone();
    void scanAudioVaeDone();
    void scanDone();

private:
    void scanCheckpoints();
    void scanDiffusionModels();
    void scanTextEncoders();
    void scanLoras();
    void scanEmbeddings();
    void scanControlNet();
    void scanUpscaleModel();
    void scanVaeModel();
    void scanAudioVaeModel();
    void scan(QmlStringList *dirs, QmlStringList *files,
              const QStringList &allowedSuffix = {"safetensor, gguf"});

};

#endif // QAIPATHING_H
