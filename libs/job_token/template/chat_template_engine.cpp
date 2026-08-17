#include "template/chat_template_engine.h"

#include <iostream>
#include <numeric>
#include <utility>

#include "template/jinja_lexer.h"
#include "template/jinja_parser.h"
#include "template/jinja_vm.h"

namespace job::token {

ChatTemplateEngine::ChatTemplateEngine(ChatTemplateType type)
    : m_type{type}
{
}

ChatTemplateEngine::ChatTemplateEngine(std::string customTemplate)
    : m_type{detectTemplateType(customTemplate)},
      m_templateStr{std::move(customTemplate)}
{
    compileTemplate();
}

ChatTemplateType ChatTemplateEngine::detectTemplateType(std::string_view jinjaStr) noexcept
{
    if (jinjaStr.empty())
        return ChatTemplateType::ChatML;

    if (jinjaStr.find("<|im_start|>") != std::string_view::npos) {
        return ChatTemplateType::ChatML;
    }
    if (jinjaStr.find("<|start_header_id|>") != std::string_view::npos) {
        return ChatTemplateType::LLaMA3;
    }
    if (jinjaStr.find("<start_of_turn>") != std::string_view::npos) {
        return ChatTemplateType::Gemma;
    }
    if (jinjaStr.find("[INST]") != std::string_view::npos) {
        return ChatTemplateType::Mistral;
    }

    return ChatTemplateType::Custom;
}

void ChatTemplateEngine::setTemplateString(std::string templateStr)
{
    m_templateStr = std::move(templateStr);
    m_type = detectTemplateType(m_templateStr);
    compileTemplate();
}

void ChatTemplateEngine::compileTemplate()
{
    m_compiledAst.reset();
    if (m_templateStr.empty()) {
        return;
    }

    try {
        JinjaLexer lexer(m_templateStr);
        auto tokens = lexer.tokenizeAll();
        JinjaParser parser(tokens);
        m_compiledAst = parser.parse();
    } catch (...) {
        // If compilation fails, m_compiledAst remains null and apply()
        // gracefully falls back to canonical formatters or generic formatting.
        m_compiledAst.reset();
    }
}

std::string ChatTemplateEngine::apply(
    std::span<const ChatMessage> messages,
    bool addGenerationPrompt,
    std::string_view bosToken,
    std::string_view eosToken) const
{

    std::cout
        << "[CHAT LOOK] type="
        << static_cast<int>(m_type)
        << " compiled="
        << (m_compiledAst ? "yes" : "no")
        << '\n';
    // If a compiled Jinja AST is available, execute via VM
    if (m_compiledAst) {

        // JOSEPH REVISTIT BROKEN
        // return formatJinja(messages, addGenerationPrompt, bosToken, eosToken);
    }

    // Fast-path / canonical fallbacks
    switch (m_type) {
    case ChatTemplateType::ChatML:
        return formatChatML(messages, addGenerationPrompt, bosToken);
    case ChatTemplateType::LLaMA3:
        return formatLLaMA3(messages, addGenerationPrompt, bosToken);
    case ChatTemplateType::Gemma:
        return formatGemma(messages, addGenerationPrompt, bosToken);
    case ChatTemplateType::Mistral:
        return formatMistral(messages, addGenerationPrompt, bosToken, eosToken);
    case ChatTemplateType::Custom:
        return formatGenericCustom(messages, addGenerationPrompt, bosToken, eosToken);
    case ChatTemplateType::Raw:
    default: {
        std::string raw;
        for (const auto& msg : messages) {
            raw.append(msg.content);
            raw.push_back('\n');
        }
        return raw;
    }
    }
}

std::string ChatTemplateEngine::formatJinja(
    std::span<const ChatMessage> messages,
    bool addGenPrompt,
    std::string_view bosToken,
    std::string_view eosToken) const
{
    if (!m_compiledAst) {
        return formatGenericCustom(messages, addGenPrompt, bosToken, eosToken);
    }

    // Prepare Jinja context variables
    ValueMap context;

    ValueList messagesList;
    messagesList.reserve(messages.size());
    for (const auto& msg : messages) {
        ValueMap msgMap;
        msgMap["role"] = Value(std::string(msg.roleName()));
        msgMap["content"] = Value(msg.content);
        if (!msg.name.empty()) {
            msgMap["name"] = Value(msg.name);
        }
        messagesList.emplace_back(std::move(msgMap));
    }

    context["messages"]              = Value(std::move(messagesList));
    context["add_generation_prompt"] = Value(addGenPrompt);
    context["bos_token"]             = Value(std::string(bosToken));
    context["eos_token"]             = Value(std::string(eosToken));

    JinjaVM vm;
    return vm.execute(*m_compiledAst, context);
}

// ChatML: <|im_start|>role\ncontent<|im_end|>\n
std::string ChatTemplateEngine::formatChatML(
    std::span<const ChatMessage> messages,
    bool addGenPrompt,
    std::string_view bosToken) const
{
    std::string result;
    if (!bosToken.empty()) {
        result.append(bosToken);
    }

    for (const auto& msg : messages) {
        result.append("<|im_start|>");
        result.append(msg.roleName());
        result.push_back('\n');
        result.append(msg.content);
        result.append("<|im_end|>\n");
    }

    if (addGenPrompt) {
        result.append("<|im_start|>assistant\n");
    }

    return result;
}

// LLaMA-3 / Qwen-2.5: <|start_header_id|>role<|end_header_id|>\n\ncontent<|eot_id|>
std::string ChatTemplateEngine::formatLLaMA3(
    std::span<const ChatMessage> messages,
    bool addGenPrompt,
    std::string_view bosToken) const
{
    std::string result;
    if (!bosToken.empty()) {
        result.append(bosToken);
    } else {
        result.append("<|begin_of_text|>");
    }

    for (const auto& msg : messages) {
        result.append("<|start_header_id|>");
        result.append(msg.roleName());
        result.append("<|end_header_id|>\n\n");
        result.append(msg.content);
        result.append("<|eot_id|>");
    }

    if (addGenPrompt) {
        result.append("<|start_header_id|>assistant<|end_header_id|>\n\n");
    }

    return result;
}

// Gemma: <start_of_turn>role\ncontent<end_of_turn>\n
std::string ChatTemplateEngine::formatGemma(
    std::span<const ChatMessage> messages,
    bool addGenPrompt,
    std::string_view bosToken) const
{
    std::string result;
    if (!bosToken.empty()) {
        result.append(bosToken);
    }

    for (const auto& msg : messages) {
        result.append("<start_of_turn>");
        // Gemma maps 'assistant' -> 'model'
        if (msg.role == ChatRole::Assistant) {
            result.append("model");
        } else {
            result.append(msg.roleName());
        }
        result.push_back('\n');
        result.append(msg.content);
        result.append("<end_of_turn>\n");
    }

    if (addGenPrompt) {
        result.append("<start_of_turn>model\n");
    }

    return result;
}

// Mistral / LLaMA-2: [INST] prompt [/INST]
std::string ChatTemplateEngine::formatMistral(
    std::span<const ChatMessage> messages,
    bool addGenPrompt,
    std::string_view bosToken,
    std::string_view eosToken) const
{
    std::string result;
    std::string sysPrompt;

    for (const auto& msg : messages) {
        if (msg.role == ChatRole::System) {
            sysPrompt = msg.content;
            break;
        }
    }

    bool isFirstUser = true;

    for (const auto& msg : messages) {
        if (msg.role == ChatRole::System) {
            continue;
        }

        if (msg.role == ChatRole::User) {
            if (!bosToken.empty()) {
                result.append(bosToken);
            }
            result.append("[INST] ");
            if (isFirstUser && !sysPrompt.empty()) {
                result.append(sysPrompt);
                result.append("\n\n");
                isFirstUser = false;
            }
            result.append(msg.content);
            result.append(" [/INST]");
        } else if (msg.role == ChatRole::Assistant) {
            result.push_back(' ');
            result.append(msg.content);
            if (!eosToken.empty()) {
                result.append(eosToken);
            } else {
                result.append("</s>");
            }
        }
    }

    if (addGenPrompt) {
        result.push_back(' ');
    }

    return result;
}

// Generic fallback formatter
std::string ChatTemplateEngine::formatGenericCustom(
    std::span<const ChatMessage> messages,
    bool addGenPrompt,
    std::string_view bosToken,
    std::string_view /* eosToken */) const
{
    std::string result;
    if (!bosToken.empty()) {
        result.append(bosToken);
    }

    for (const auto& msg : messages) {
        result.append("### ");
        result.append(msg.roleName());
        result.append(":\n");
        result.append(msg.content);
        result.append("\n\n");
    }

    if (addGenPrompt) {
        result.append("### assistant:\n");
    }

    return result;
}

} // namespace job::token