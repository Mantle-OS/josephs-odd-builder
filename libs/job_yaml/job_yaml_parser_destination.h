#pragma once

#include <cstddef>
#include <cstdint>
#include <inplace_vector>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "job_yaml_concepts.h"
#include "job_yaml_indent_stack.h"
#include "job_yaml_node.h"
#include "job_yaml_node_sink.h"

namespace job::yaml {

class YamlNodeParserDestination
{
public:
    using Ptr = std::shared_ptr<YamlNodeParserDestination>;
    using WPtr = std::weak_ptr<YamlNodeParserDestination>;
    using UPtr = std::unique_ptr<YamlNodeParserDestination>;

    explicit YamlNodeParserDestination(YamlNode &destination) noexcept :
        m_destination(&destination)
    {
    }

    ~YamlNodeParserDestination() = default;

    YamlNodeParserDestination(const YamlNodeParserDestination &) = delete;
    YamlNodeParserDestination &operator=(const YamlNodeParserDestination &) = delete;
    YamlNodeParserDestination(YamlNodeParserDestination &&) noexcept = default;
    YamlNodeParserDestination &operator=(YamlNodeParserDestination &&) noexcept = default;

    template <typename... Args>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<YamlNodeParserDestination>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<YamlNodeParserDestination>(std::forward<Args>(args)...);
    }

    [[nodiscard]] bool null()
    {
        if (m_complete)
            return false;

        if (m_frames.empty()) {
            if (!YamlNodeSink::null(*m_destination))
                return false;

            m_complete = true;
            return true;
        }

        YamlNode value;
        if (!YamlNodeSink::null(value))
            return false;

        Frame &frame = m_frames.back();

        if (frame.context == Context::Mapping) {
            if (!frame.hasKey)
                return false;

            if (!YamlNodeSink::member(frame.node, frame.key(), std::move(value)))
                return false;

            frame.clearKey();
            return true;
        }

        return YamlNodeSink::append(frame.node, std::move(value));
    }

    [[nodiscard]] bool scalar(std::string_view value)
    {
        if (m_complete)
            return false;

        if (m_frames.empty()) {
            if (!YamlNodeSink::scalarView(*m_destination, value))
                return false;

            m_complete = true;
            return true;
        }

        Frame &frame = m_frames.back();

        if (frame.context == Context::Mapping) {
            if (!frame.hasKey)
                return false;

            if (!YamlNodeSink::memberView(frame.node, frame.key(), value))
                return false;

            frame.clearKey();
            return true;
        }

        return YamlNodeSink::appendView(frame.node, value);
    }

    [[nodiscard]] bool scalarOwned(std::string value)
    {
        if (m_complete)
            return false;

        if (m_frames.empty()) {
            if (!YamlNodeSink::scalar(*m_destination, value))
                return false;

            m_complete = true;
            return true;
        }

        Frame &frame = m_frames.back();

        if (frame.context == Context::Mapping) {
            if (!frame.hasKey)
                return false;

            if (!YamlNodeSink::member(frame.node, frame.key(), value))
                return false;

            frame.clearKey();
            return true;
        }

        return YamlNodeSink::append(frame.node, value);
    }

    [[nodiscard]] bool key(std::string_view key) noexcept
    {
        if (m_complete || m_frames.empty())
            return false;

        Frame &frame = m_frames.back();

        if (frame.context != Context::Mapping || frame.hasKey)
            return false;

        frame.keyView = key;
        frame.keyStorage.clear();
        frame.keyOwned = false;
        frame.hasKey = true;
        return true;
    }

    [[nodiscard]] bool keyOwned(std::string key)
    {
        if (m_complete || m_frames.empty())
            return false;

        Frame &frame = m_frames.back();

        if (frame.context != Context::Mapping || frame.hasKey)
            return false;

        frame.keyView = {};
        frame.keyStorage = std::move(key);
        frame.keyOwned = true;
        frame.hasKey = true;
        return true;
    }

    [[nodiscard]] bool beginMapping()
    {
        return beginContainer(Context::Mapping);
    }

    [[nodiscard]] bool endMapping()
    {
        return endContainer(Context::Mapping);
    }

    [[nodiscard]] bool beginSequence()
    {
        return beginContainer(Context::Sequence);
    }

    [[nodiscard]] bool endSequence()
    {
        return endContainer(Context::Sequence);
    }

    [[nodiscard]] bool complete() const noexcept
    {
        return m_complete;
    }

    [[nodiscard]] std::size_t depth() const noexcept
    {
        return m_frames.size();
    }

private:
    enum class Context : std::uint8_t {
        Mapping,
        Sequence
    };

    struct Frame
    {
        Context context;
        YamlNode node;
        std::string_view keyView;
        std::string keyStorage;
        bool keyOwned{};
        bool hasKey{};

        [[nodiscard]] std::string_view key() const noexcept
        {
            return keyOwned ? std::string_view{keyStorage} : keyView;
        }

        void clearKey() noexcept
        {
            keyView = {};
            keyStorage.clear();
            keyOwned = false;
            hasKey = false;
        }
    };

    using Frames = std::inplace_vector<Frame, YamlIndentStack::MaxDepth>;

    [[nodiscard]] bool beginContainer(Context context)
    {
        if (m_complete || m_frames.size() == m_frames.capacity())
            return false;

        if (!m_frames.empty()) {
            const Frame &parent = m_frames.back();

            if (parent.context == Context::Mapping && !parent.hasKey)
                return false;
        }

        Frame frame{
            .context = context,
            .node = {},
            .keyView = {},
            .keyStorage = {},
            .keyOwned = false,
            .hasKey = false
        };

        const bool initialized = context == Context::Mapping ? YamlNodeSink::mapping(frame.node) : YamlNodeSink::sequence(frame.node);

        if (!initialized)
            return false;

        m_frames.push_back(std::move(frame));
        return true;
    }

    [[nodiscard]] bool endContainer(Context context)
    {
        if (m_complete || m_frames.empty())
            return false;

        Frame &frame = m_frames.back();

        if (frame.context != context)
            return false;

        if (frame.context == Context::Mapping && frame.hasKey)
            return false;

        YamlNode completed = std::move(frame.node);
        m_frames.pop_back();

        if (m_frames.empty()) {
            *m_destination = std::move(completed);
            m_complete = true;
            return true;
        }

        Frame &parent = m_frames.back();

        if (parent.context == Context::Mapping) {
            if (!parent.hasKey)
                return false;

            if (!YamlNodeSink::member(parent.node, parent.key(), std::move(completed)))
                return false;

            parent.clearKey();
            return true;
        }

        return YamlNodeSink::append(parent.node, std::move(completed));
    }

    YamlNode *m_destination{};
    Frames m_frames;
    bool m_complete{};
};

static_assert(YamlParseDestination<YamlNodeParserDestination>);

} // namespace job::yaml