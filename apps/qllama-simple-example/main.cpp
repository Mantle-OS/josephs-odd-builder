#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include <qllamamodel.h>
#include <qllamamodelparams.h>
#include <qllmacontext.h>
#include <qllamacontextparams.h>
#include <qllamasampler.h>
#include <qllamalocalengine.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    auto *modelParams = new QLlamaModelParams(&app);
    modelParams->set_nGpuLayers(99); // Offload as many layers to CUDA cores as possible

    auto *model = new QLlamaModel(&app);
    model->set_modelPath("/srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf");

    if (!model->loadModel(modelParams)) {
        qCritical() << "Model Loading Aborted:" << model->get_lastErrorString();
        return -1;
    }

    auto *ctxParams = new QLlamaContextParams(&app);
    ctxParams->set_nCtx(2048);
    ctxParams->set_nThreads(8);
    ctxParams->set_nThreadsBatch(8);

    auto *context = new QLlamaContext(&app);
    if (!context->initContext(model, ctxParams)) {
        qCritical() << "Context Allocation Aborted:" << context->get_lastErrorString();
        return -1;
    }

    auto *sampler = new QLlamaSampler(&app);
    sampler->set_temperature(0.75);
    sampler->set_topP(0.90);
    sampler->set_topK(40);

    // Tie everything into the "high-performance" local streaming engine
    auto *engine = new QLlamaLocalEngine(&app);
    engine->set_model(model);
    engine->set_context(context);
    engine->set_sampler(sampler);

    QObject::connect(engine, &QLlamaEngine::generationStarted, []() {
        qDebug() << "\n[Inference Started] Prompt processing complete. Response Stream:";
    });

    QObject::connect(engine, &QLlamaEngine::generationFinished, [&app](const QString &fullText) {
        qDebug() << "\n----------------------------------------------------";
        qDebug() << "[Inference Finished] Total Text Length Compiled:" << fullText.length() << "characters.";
        qDebug() << "Closing execution environment context.";
        app.quit(); // Gracefully drop out of Qt event queue loop
    });

    QObject::connect(engine, &QLlamaEngine::executionError, [&app](const QString &err) {
        qCritical() << "\n[Engine Processing Error Failure Encountered]:" << err;
        app.quit();
    });

    engine->set_prompt("<|im_start|>user\nExplain why minimal software architectures with fewer third-party dependencies are inherently more robust.<|im_end|>\n<|im_start|>assistant\n");
    QTimer::singleShot(100, engine, &QLlamaEngine::generate);

    return app.exec();
}
