#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <chrono>
#include <iomanip>

#include <job_tmp_file.h>

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

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    std::cout << "\n=== JOB Inference Baseline ===\n";
    std::cout << std::fixed << std::setprecision(3);

    using Clock = std::chrono::steady_clock;
    const auto programStart = Clock::now();
    const auto elapsedMs = [](Clock::time_point start, Clock::time_point end) {
        return std::chrono::duration<double, std::milli>(end - start).count();
    };


    const std::filesystem::path tokenizerDir{ "/home/jmills/git/opensource/qwen_tmp" };
    const std::filesystem::path tokenizerJson = tokenizerDir / "tokenizer.json";
    const std::filesystem::path tokenizerConfig = tokenizerDir / "tokenizer_config.json";

    // Just for now. Obv this will or should be changed later on
    const std::filesystem::path modelPath{
        "/srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/"
        "Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
    };

    constexpr std::uint32_t InferenceContext = 4096;
    std::cout << "Inference context: " << InferenceContext << '\n';

    constexpr std::int32_t MaxNewTokens = 256;
    std::cout << "Max new tokens:    " << MaxNewTokens << '\n';


    if (!std::filesystem::exists(modelPath)) {
        std::cerr << "GGUF model not found: " << modelPath << '\n';
        return 1;
    }

    const auto tokenizerStart = Clock::now();
    job::token::JobToken tokenizer;
    if (!tokenizer.load(job::token::IToken::Provider::HuggingFace, tokenizerConfig, tokenizerJson)) {
        std::cerr << "Failed to load tokenizer\n";
        return 1;
    }
    const auto tokenizerEnd = Clock::now();
    std::cout << "Tokenizer load:     " << elapsedMs(tokenizerStart, tokenizerEnd) << " ms\n";


    const auto deviceStart = Clock::now();
    job::ggml::JobGgml interface{false};
    // interface.setLogCallback(&shutUp);

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

    const auto deviceEnd = Clock::now();
    std::cout << "Device setup:       " << elapsedMs(deviceStart, deviceEnd) << " ms\n";
    std::cout << device->dump();


    const auto schedulerStart = Clock::now();
    // auto scheduler = manager->buildScheduler(device, 8192);
    auto scheduler = manager->buildScheduler(device, 4096);
    if (!scheduler || !scheduler->isValid()) {
        std::cerr << "Failed to create device scheduler\n" << device->dump();
        return 1;
    }
    const auto schedulerEnd = Clock::now();
    std::cout << "Scheduler creation: " << elapsedMs(schedulerStart, schedulerEnd) << " ms\n";


    const auto configStart = Clock::now();
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
    samplerConfig.setGreedy(true); // test both obv false will be slower but whatever
    std::cout << "Sampler:           " << (samplerConfig.greedy() ? "greedy" : "not greedy") << '\n';
    samplerConfig.setSeed(42);
    modelConfig.debugJobJson();
    const auto configEnd = Clock::now();
    std::cout << "Model config:       " << elapsedMs(configStart, configEnd) << " ms\n";



    const auto chatMessageStart = Clock::now();
    job::token::ChatMessage systemMessage;
    systemMessage.setRole(job::token::ChatRole::System);
    systemMessage.setContent(
        "You are an expert at writing poetry. "
        "Your goal is to provide poetry without unnecessary explanations. "
        "Your response must fit within " +
        std::to_string(MaxNewTokens) +
        " tokens.");
    job::token::ChatMessage userMessage;
    userMessage.setRole(job::token::ChatRole::User);
    userMessage.setContent(
        "Write a beautiful poem about when electricity and vectors line up to"
        "create a sense of wonder");
    const std::vector<job::token::ChatMessage> dialogue{ systemMessage, userMessage };
    const auto chatMessageEnd = Clock::now();
    std::cout << "ChatMessage         " << elapsedMs(chatMessageStart, chatMessageEnd) << " ms\n";


    const auto promptStart = Clock::now();
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
    const auto promptEnd = Clock::now();
    std::cout << "Prompt preparation: " << elapsedMs(promptStart, promptEnd) << " ms\n";


    const auto modelLoadStart = Clock::now();
    job::model::JobModel model{ *device, *scheduler };
    if (!model.load(modelPath, modelConfig, InferenceContext)) {
        std::cerr << "Failed to load model\n";
        return 1;
    }
    model.setTokenizer(&tokenizer);
    const auto modelLoadEnd = Clock::now();
    std::cout << "Model load:         " << elapsedMs(modelLoadStart, modelLoadEnd) << " ms\n";


    const auto generationStart = Clock::now();
    const std::vector<job::token::TokenId> generatedTokens = model.generate(promptTokens, MaxNewTokens, samplerConfig);
    std::cout << "Prompt tokens:      " << promptTokens.size() << '\n';
    if (generatedTokens.size() <= promptTokens.size()) {
        std::cerr << "Generation failed\n";
        return 1;
    }

    const auto responseBegin = generatedTokens.begin() + static_cast<std::ptrdiff_t>(promptTokens.size());
    const std::vector<job::token::TokenId> responseTokens{ responseBegin, generatedTokens.end() };
    const auto generationEnd = Clock::now();
    std::cout << "Generation:         " << elapsedMs(generationStart, generationEnd) << " ms\n";
    const std::size_t generatedCount = generatedTokens.size() >= promptTokens.size() ? generatedTokens.size() - promptTokens.size() : 0;
    std::cout << "Generated tokens:   " << generatedCount << '\n';
    const double generationSeconds = std::chrono::duration<double>(generationEnd - generationStart).count();

    const auto decodeStart = Clock::now();
    const std::string responseText = tokenizer.decode(responseTokens);
    const auto decodeEnd = Clock::now();
    std::cout << "Generation:         " << elapsedMs(decodeStart, decodeEnd) << " ms\n";
    std::cout << "Generation rate:    " << (generationSeconds > 0.0 ? generatedCount / generationSeconds : 0.0) << " tokens/s\n";



    std::cout << "Model Timing:\n";
    const auto &timing = model.inferenceTiming();
    std::cout << "  Prefill:\n";
    std::cout << "    Reset:   " << timing.prefill.reset.averageMs() << " ms\n";
    std::cout << "    Input:   " << timing.prefill.input.averageMs() << " ms\n";
    std::cout << "    Build:   " << timing.prefill.build.averageMs() << " ms\n";
    std::cout << "    Compute: " << timing.prefill.compute.averageMs() << " ms\n";
    std::cout << "    Sync:    " << timing.prefill.sync.averageMs() << " ms\n";
    std::cout << "    Sample:  " << timing.prefill.sample.averageMs() << " ms\n";

    std::cout << "  Decode:\n";
    std::cout << "    Reset:   " << timing.decode.reset.averageMs()         << " ms\n";
    std::cout << "    Input:   " << timing.decode.input.averageMs()         << " ms\n";
    std::cout << "    Build:   " << timing.decode.build.averageMs()         << " ms\n";
    std::cout << "    Alloc:   " << timing.decode.computeAlloc.averageMs()  << " ms\n";
    std::cout << "    Compute: " << timing.decode.compute.averageMs()       << " ms\n";
    std::cout << "    Sync:    " << timing.decode.sync.averageMs()          << " ms\n";
    std::cout << "    Sample:  " << timing.decode.sample.averageMs()        << " ms\n";

    std::cout << "Total elapsed:      " << elapsedMs(programStart, Clock::now()) << " ms\n";
    std::cout << "====================\n";





    std::cout << "Response:\n" << responseText << '\n';


    return 0;
}