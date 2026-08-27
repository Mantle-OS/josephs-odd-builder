#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <job_ggml.h>
#include <job_ggml_backend_sched.h>
#include <job_ggml_device_manager.h>

#include <job_model.h>
#include <job_token.h>

#include <chat/chat_message.h>
#include <config/arch/qwen/qwen3_instruct_2507.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    const std::filesystem::path tokenizerDir{ "/home/jmills/git/opensource/qwen_tmp" };
    const std::filesystem::path tokenizerJson = tokenizerDir / "tokenizer.json";
    const std::filesystem::path tokenizerConfig = tokenizerDir / "tokenizer_config.json";
    const std::filesystem::path modelPath{
        "/srv/ai/ComfyUI/models/text_encoders/"
        "ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
    };

    constexpr std::uint32_t InferenceContext = 4096;
    constexpr std::int32_t MaxNewTokens = 256;


    if (!std::filesystem::exists(modelPath)) {
        std::cerr << "GGUF model not found: " << modelPath << '\n';
        return 1;
    }

    job::token::JobToken tokenizer;
    if (!tokenizer.load(job::token::IToken::Provider::HuggingFace, tokenizerConfig, tokenizerJson)) {
        std::cerr << "Failed to load tokenizer\n";
        return 1;
    }

    job::ggml::JobGgml interface;
    job::ggml::JobGgmlDeviceManager *manager = interface.deviceManager();

    if (!manager || !manager->isValid()) {
        std::cerr << "Device manager unavailable\n";
        return 1;
    }

    job::ggml::JobGgmlCpu *cpu = manager->cpu();
    if (!cpu) {
        std::cerr << "No CPU GGML device\n";
        return 1;
    }

    auto scheduler = manager->buildScheduler(cpu, 4096);
    if (!scheduler || !scheduler->isValid()) {
        std::cerr << "Failed to create CPU scheduler\n";
        return 1;
    }

    job::model::arch::qwen::Qwen3Instruct2507Config presetConfig;
    if (!presetConfig.isValid()) {
        std::cerr << "Qwen3 preset failed validation\n";
        return 1;
    }

    //  lets test it now.
    const auto json = presetConfig.toJson();
    presetConfig.debugJson();


    job::model::SamplerConfig samplerConfig = presetConfig.samplerConfig();
    samplerConfig.setGreedy(true);

    job::model::JobModel model{ *cpu, *scheduler };
    if (!model.load(modelPath, presetConfig, InferenceContext)) {
        std::cerr << "Failed to load model\n";
        return 1;
    }


    job::token::ChatMessage systemMessage;
    systemMessage.setRole(job::token::ChatRole::System);
    systemMessage.setContent(
        "You are an expert C++ systems programmer. "
        "Provide high-performance, clean code without unnecessary explanations. "
        "Your response must fit within " +
        std::to_string(MaxNewTokens) +
        " tokens.");

    job::token::ChatMessage userMessage;
    userMessage.setRole(job::token::ChatRole::User);
    userMessage.setContent(
        "Write a high-performance C++ function that computes "
        "the inverse square root of a float.");

    const std::vector<job::token::ChatMessage> dialogue{ systemMessage, userMessage };
    const std::string formattedPrompt = tokenizer.applyChatMessages(dialogue, true);

    if (formattedPrompt.empty()) {
        std::cerr << "Failed to apply chat messages\n";
        return 1;
    }

    const std::vector<job::token::TokenId> promptTokens = tokenizer.encode(formattedPrompt);
    if (promptTokens.empty()) {
        std::cerr << "Tokenizer produced no tokens\n";
        return 1;
    }

    const std::vector<job::token::TokenId> generatedTokens = model.generate(promptTokens, MaxNewTokens, samplerConfig);
    if (generatedTokens.size() <= promptTokens.size()) {
        std::cerr << "Generation failed\n";
        return 1;
    }

    const auto responseBegin = generatedTokens.begin() + static_cast<std::ptrdiff_t>(promptTokens.size());
    const std::vector<job::token::TokenId> responseTokens{ responseBegin, generatedTokens.end() };

    const std::string responseText = tokenizer.decode(responseTokens);
    std::cout << responseText << '\n';

    return 0;
}