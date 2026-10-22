#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>


#include <job_ggml.h>
#include <job_ggml_backend_sched.h>
#include <job_ggml_device_manager.h>

#include <job_model.h>
#include <job_token.h>

#include <chat/chat_message.h>
// #include <config/arch/qwen/qwen3_instruct_2507.h>
#include <config/model_config.h>
#include <io/gguf_model_config_reader.h>


void shutUp(ggml_log_level, const char *, void *)
{
    return;
}

// bool isToolCallStart(job::token::TokenId tokenId) const noexcept
// {
//     return tokenId == 151657;
// }


// bool isToolCallEnd(job::token::TokenId tokenId) const noexcept
// {
//     return tokenId == 151658;
// }

// bool isToolRespStart(job::token::TokenId tokenId)
// {
//     return tokenId == 151665;
// }

// bool isToolRespEnd(job::token::TokenId tokenId)
// {
//     return tokenId == 151666;
// }


int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    const std::filesystem::path tokenizerDir{ "/home/jmills/git/opensource/qwen_tmp" };
    const std::filesystem::path tokenizerJson = tokenizerDir / "tokenizer.json";
    const std::filesystem::path tokenizerConfig = tokenizerDir / "tokenizer_config.json";

    // Just for now. Obv this will or should be changed later on
    const std::filesystem::path modelPath{
        "/srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/"
        "Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
    };

    // constexpr std::uint32_t InferenceContext = 4096;
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

    job::ggml::JobGgml interface{false};
    interface.setLogCallback(&shutUp);

    job::ggml::JobGgmlDeviceManager *manager = interface.deviceManager();

    if (!manager) {
        std::cerr << "Device manager unavailable\n";
        return 1;
    }

    manager->scan();
    if(!manager->isValid()){
        std::cerr << "Device manager Has no devices\n";
        return 1;
    }


    job::ggml::JobGgmlDevice *device = nullptr;
    if(!manager->cudaDevices().isEmpty())
        device = manager->cudaDevices().at(0);
    else
        device = manager->cpu();

    if (!device) {
        std::cerr << "No CPU Or GPU GGML device\n";
        return 1;
    }
#ifndef NDEBUG
    std::cout << device->dump();
#endif

    auto scheduler = manager->buildScheduler(device, 8192);
    if (!scheduler || !scheduler->isValid()) {
        std::cerr << "Failed to create CPU or GPU scheduler\n";
        return 1;
    }

    job::model::GgufModelConfigReader reader;
    job::model::ModelConfig modelConfig;
    if (!reader.read(modelPath, modelConfig)) {
        std::cerr << "Failed to read GGUF model configuration\n";
        return 1;
    }

    if (!modelConfig.isValid()) {
        std::cerr << "Invalid GGUF model configuration\n";
        return 1;
    }

    job::model::SamplerConfig samplerConfig = modelConfig.samplerConfig();
    samplerConfig.setGreedy(false);
    samplerConfig.setSeed(42);
#ifndef NDEBUG
    modelConfig.debugJobJson();
#endif


    job::token::ChatMessage toolMessage;
    toolMessage.setRole(job::token::ChatRole::Tool);
    toolMessage.setContent("/tools");


    job::token::ChatMessage systemMessage;
    systemMessage.setRole(job::token::ChatRole::System);
    systemMessage.setContent(
        "You have access to a tool named tools.\n"
        "It returns a list of tools available in this session.\n"
        "It takes no arguments.\n"
        "When the user asks what tools are available, call it.\n"
        );

    job::token::ChatMessage userMessage;
    userMessage.setRole(job::token::ChatRole::User);
    userMessage.setContent("Tell me what tools you have");

    const std::vector<job::token::ChatMessage> dialogue{ systemMessage, userMessage, toolMessage };
    const std::string formattedPrompt = tokenizer.applyChatMessages(dialogue, true);

    std::cout << "\n--- CHAT TEMPLATE OUTPUT ---\n"
              << formattedPrompt
              << "\n--- END CHAT TEMPLATE ---\n";


    if (formattedPrompt.empty()) {
        std::cerr << "Failed to apply chat messages\n";
        return 1;
    }

    const std::vector<job::token::TokenId> promptTokens = tokenizer.encode(formattedPrompt);
    if (promptTokens.empty()) {
        std::cerr << "Tokenizer produced no tokens\n";
        return 1;
    }


    // Okay now taht the Chat deal is setup we should pass it into the model ?
    job::model::JobModel model{ *device, *scheduler };
    if (!model.load(modelPath, modelConfig, InferenceContext)) {
        std::cerr << "Failed to load model\n";
        return 1;
    }

    // model.setTokenizer(&tokenizer); // just for now so it does not stop the messages and we get the full output

    // Hook up the new library for the tools.
    // job::aitools::JobAiTool tools;
    // tools.call(the token)




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