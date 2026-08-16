#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <job_ggml_backend_sched.h>
#include <job_ggml_device_manager.h>

#include <job_model.h>
#include <job_tokenizer.h>

#include <config/arch/qwen/qwen3_instruct_2507.h>
#include <template/chat_message.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    const std::filesystem::path modelPath{
        "/srv/ai/ComfyUI/models/text_encoders/"
        "ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
    };

    const std::filesystem::path tokenizerDir{
        "/home/jmills/git/opensource/qwen_tmp"
    };

    const std::filesystem::path tokenizerJson =
        tokenizerDir / "tokenizer.json";

    const std::filesystem::path tokenizerConfig =
        tokenizerDir / "tokenizer_config.json";

    //
    // ------------------------------------------------------------------------
    // Tokenizer
    // ------------------------------------------------------------------------
    //

    if (!std::filesystem::exists(modelPath)) {
        std::cerr
            << "[JobInferenceTest] GGUF model not found: "
            << modelPath
            << '\n';

        return 1;
    }

    job::token::JobTokenizer tokenizer;

    std::cout
        << "[JobInferenceTest] Loading tokenizer from "
        << tokenizerDir
        << "...\n";

    if (!tokenizer.loadHf(tokenizerJson, tokenizerConfig)) {
        std::cerr
            << "[JobInferenceTest] Failed to load tokenizer\n";

        return 1;
    }

    std::cout
        << "[JobInferenceTest] Tokenizer loaded. Vocab size: "
        << tokenizer.vocabSize()
        << '\n';

    //
    // ------------------------------------------------------------------------
    // Resolve runtime.
    // ------------------------------------------------------------------------
    //

    job::ggml::JobGgmlDeviceManager manager;

    if (!manager.isValid()) {
        std::cerr
            << "[JobInferenceTest] Device manager failed: "
            << manager.errorString()
            << '\n';

        return 1;
    }

    job::ggml::JobGgmlCpu *cpu =
        manager.cpu();

    if (!cpu) {
        std::cerr
            << "[JobInferenceTest] No CPU GGML device\n";

        return 1;
    }

    auto scheduler = manager.buildScheduler(cpu, 4096);
    if (!scheduler || !scheduler->isValid()) {
        std::cerr << "[JobInferenceTest] Failed to create CPU scheduler\n";
        return 1;
    }

    std::cout
        << "[JobInferenceTest] Runtime resolved: "
        << cpu->uid()
        << '\n';

    //
    // ------------------------------------------------------------------------
    // Model configuration.
    // ------------------------------------------------------------------------
    //

    job::model::arch::qwen::Qwen3Instruct2507Config presetConfig;

    if (!presetConfig.isValid()) {
        std::cerr
            << "[JobInferenceTest] Qwen3 preset failed validation\n";

        return 1;
    }

    constexpr uint32_t InferenceContext = 4096;

    //
    // ------------------------------------------------------------------------
    // Model.
    // ------------------------------------------------------------------------
    //

    job::model::JobModel model{
        *cpu,
        *scheduler
    };

    std::cout
        << "[JobInferenceTest] Loading "
        << modelPath.filename()
        << "...\n";

    if (!model.load(
            modelPath,
            presetConfig,
            InferenceContext)) {

        std::cerr
            << "[JobInferenceTest] Failed to load model\n";

        return 1;
    }

    std::cout
        << "[JobInferenceTest] Model loaded\n"
        << "  architecture: "
        << model.config().architectureName()
        << '\n'
        << "  layers: "
        << model.config().transformerConfig().blockCount()
        << '\n'
        << "  context: "
        << model.kvCache()->maxContextLength()
        << '\n';

    //
    // ------------------------------------------------------------------------
    // Chat template.
    // ------------------------------------------------------------------------
    //

    std::vector<job::token::ChatMessage> dialogue{
        {
            job::token::ChatRole::System,
            "You are an expert C++ systems programmer. "
            "Provide high-performance, clean code without unnecessary explanations.",
            "",
            ""
        },
        {
            job::token::ChatRole::User,
            "Write a high-performance C++ function that computes "
            "the inverse square root of a float.",
            "",
            ""
        }
    };

    const std::string formattedPrompt =
        tokenizer.applyChatTemplate(
            dialogue,
            true);

    std::cout
        << "\n[JobInferenceTest] Formatted prompt:\n"
        << formattedPrompt
        << "\n\n";

    const std::vector<int32_t> promptTokens =
        tokenizer.encode(
            formattedPrompt,
            false,
            false);

    if (promptTokens.empty()) {
        std::cerr
            << "[JobInferenceTest] Tokenizer produced no tokens\n";

        return 1;
    }

    std::cout
        << "[JobInferenceTest] Encoded "
        << promptTokens.size()
        << " prompt tokens\n";

    //
    // ------------------------------------------------------------------------
    // THIS IS THE KISS.
    // ------------------------------------------------------------------------
    //

    constexpr int32_t MaxNewTokens = 64;

    const std::vector<int32_t> generatedTokens =
        model.generate(
            promptTokens,
            MaxNewTokens,
            presetConfig.samplerConfig());

    if (generatedTokens.empty()) {
        std::cerr
            << "[JobInferenceTest] Generation failed\n";

        return 1;
    }

    if (generatedTokens.size() <= promptTokens.size()) {
        std::cerr
            << "[JobInferenceTest] Model produced no response tokens\n";

        return 1;
    }

    //
    // JobModel returns prompt + generated continuation.
    //
    const auto responseBegin =
        generatedTokens.begin() +
        static_cast<std::ptrdiff_t>(promptTokens.size());

    const std::vector<int32_t> responseTokens{
        responseBegin,
        generatedTokens.end()
    };

    //
    // ------------------------------------------------------------------------
    // Back through job_token.
    // ------------------------------------------------------------------------
    //

    const std::string responseText =
        tokenizer.decode(
            responseTokens,
            true);

    std::cout
        << "\n"
        << "==================================================\n"
        << "QWEN3 RESPONSE\n"
        << "==================================================\n"
        << responseText
        << '\n'
        << "==================================================\n"
        << "[JobInferenceTest] Complete\n";

    return 0;
}