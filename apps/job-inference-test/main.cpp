#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

#include <job_logger.h>
#include <job_ggml.h>
#include <job_model.h>
#include <job_tokenizer.h>
#include <config/arch/qwen/qwen3_instruct_2507.h> // Our rock-solid preset
#include <template/chat_message.h>

int main(int argc, char *argv[])
{
    // std::cout << "[JobInferenceTest] Initializing global GGML subsystem and device manager...\n";

    // // 1. Initialize process-lifetime GGML subsystem and scan for NVIDIA/CUDA devices
    // job::ggml::JobGgml ggmlSubsystem(true);
    // if (!ggmlSubsystem.isValid()) {
    //     std::cerr << "[Error] Failed to initialize JobGgml subsystem!\n";
    //     return 1;
    // }
    // std::cout << "[JobInferenceTest] GGML subsystem active. Backends initialized.\n";

    // 2. Paths to model binaries and HuggingFace tokenizer assets
    std::filesystem::path modelPath = "/srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf";
    std::filesystem::path tokenizerDir = "/home/jmills/git/opensource/qwen_tmp";
    std::filesystem::path tokenizerJson = tokenizerDir / "tokenizer.json";
    std::filesystem::path tokenizerConfig = tokenizerDir / "tokenizer_config.json";

    if (!std::filesystem::exists(modelPath)) {
        std::cerr << "[Error] GGUF model path not found: " << modelPath << "\n";
        return 1;
    }

    // 3. Load the JobTokenizer using the Hf files
    job::token::JobTokenizer tokenizer;
    std::cout << "[JobInferenceTest] Loading tokenizer from: " << tokenizerDir.string() << "...\n";

    if (!tokenizer.loadHf(tokenizerJson, tokenizerConfig)) {
        std::cerr << "[Error] Failed to load HuggingFace tokenizer files!\n";
        return 1;
    }
    std::cout << "[JobInferenceTest] Tokenizer loaded successfully! Vocab size: " << tokenizer.vocabSize() << "\n";

    job::model::arch::qwen::Qwen3Instruct2507Config presetConfig;
    if (!presetConfig.isValid()) {
        std::cerr << "[Error] Preset configuration invariant check failed!\n";
        return 1;
    }

    // Load model weights and bind tensors using the explicit preset (36 blocks)
    job::model::JobModel model;
    uint32_t inferenceCtx = 4096; // Safe VRAM working context

    if (!model.load(modelPath, presetConfig, inferenceCtx)) {
        std::cerr << "[Error] Failed to load model through JobModel facade with preset config!\n";
        return 1;
    }



    // // 4. Instantiate our static compile-time preset (Bypassing dynamic config reading entirely)
    // job::model::arch::qwen::Qwen3Instruct2507Config presetConfig;
    // if (!presetConfig.isValid()) {
    //     std::cerr << "[Error] Preset configuration invariant check failed!\n";
    //     return 1;
    // }

    // std::cout << "[JobInferenceTest] Using static preset configuration:\n"
    //           << "  - Architecture: " << presetConfig.architectureName() << "\n"
    //           << "  - Model Name:   " << presetConfig.m_archConfig.m_modelName << "\n"
    //           << "  - Block Count:  " << presetConfig.m_transformerConfig.m_blockCount << " (Exact match)\n"
    //           << "  - Embedding:    " << presetConfig.m_transformerConfig.m_embeddingLength << "\n"
    //           << "  - KV Heads:     " << presetConfig.m_transformerConfig.m_headCountKv << "\n\n";

    // // 5. Load model weights and bind them using our exact preset configuration
    // job::model::JobModel model;
    // std::cout << "[JobInferenceTest] Loading model weights from: " << modelPath.filename().string() << "...\n";

    // // If your JobModel can take a config, pass presetConfig. Otherwise, load with a safe context length (4096)
    // uint32_t inferenceCtx = 4096;
    // if (!model.load(modelPath, inferenceCtx)) {
    //     std::cerr << "[Error] Failed to load model through JobModel facade!\n";
    //     return 1;
    // }

    // 6. Construct conversation dialogue matching Qwen3's instruction format
    std::vector<job::token::ChatMessage> dialogue = {
        {
            job::token::ChatRole::System,
            "You are an expert C++ systems programmer. Provide high-performance, clean code without unnecessary explanations.",
            "", ""
        },
        {
            job::token::ChatRole::User,
            "Write a high-performance C++ function that computes the inverse square root of a float.",
            "", ""
        }
    };

    std::string formattedPrompt = tokenizer.applyChatTemplate(dialogue, true);
    std::vector<int32_t> promptTokens = tokenizer.encode(formattedPrompt, false, false);

    if (promptTokens.empty()) {
        std::cerr << "[Error] Prompt encoding failed or produced empty token stream.\n";
        return 1;
    }

    std::cout << "[JobInferenceTest] Encoded " << promptTokens.size() << " prompt tokens.\n";

    // 7. Generate response tokens using the preset's sampler config
    std::cout << "[JobInferenceTest] Generating response tokens on target device...\n";

    std::vector<int32_t> generatedTokens = model.generate(promptTokens, 512, presetConfig.m_samplerConfig);

    std::vector<int32_t> responseTokens;
    if (generatedTokens.size() > promptTokens.size()) {
        responseTokens.assign(generatedTokens.begin() + promptTokens.size(), generatedTokens.end());
    } else {
        responseTokens = generatedTokens;
    }

    std::string responseText = tokenizer.decode(responseTokens, true);

    std::cout << "\n==================================================\n";
    std::cout << "QWEN3 RESPONSE:\n";
    std::cout << "==================================================\n";
    std::cout << responseText << "\n";
    std::cout << "==================================================\n";
    std::cout << "[JobInferenceTest] Execution complete.\n";

    return 0;
}