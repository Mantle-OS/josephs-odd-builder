#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <inplace_vector>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "job_yaml_concepts.h"
#include "job_yaml_indent_stack.h"
#include "job_yaml_key_dispatch.h"
#include "job_yaml_object_reader.h"
#include "job_yaml_scalar_kernel.h"

namespace job::yaml {

template <typename T>
class YamlObjectParserDestination
{
public:
    using Ptr = std::shared_ptr<YamlObjectParserDestination>;
    using WPtr = std::weak_ptr<YamlObjectParserDestination>;
    using UPtr = std::unique_ptr<YamlObjectParserDestination>;

    explicit YamlObjectParserDestination(T &destination) noexcept :
        m_destination(&destination)
    {
    }

    ~YamlObjectParserDestination() = default;

    YamlObjectParserDestination(const YamlObjectParserDestination &) = delete;
    YamlObjectParserDestination &operator=(const YamlObjectParserDestination &) = delete;
    YamlObjectParserDestination(YamlObjectParserDestination &&) noexcept = default;
    YamlObjectParserDestination &operator=(YamlObjectParserDestination &&) noexcept = default;

    template <typename... Args>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<YamlObjectParserDestination>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<YamlObjectParserDestination>(std::forward<Args>(args)...);
    }

    [[nodiscard]] bool null() noexcept
    {
        if (m_complete || m_frames.empty())
            return false;

        Frame &frame = m_frames.back();

        if (frame.context == Context::Sequence)
            return frame.sequenceNullReader && frame.sequenceNullReader(frame.destination);

        if (!frame.hasKey || !frame.nullReader)
            return false;

        if (!frame.nullReader(frame.destination, frame.key()))
            return false;

        frame.clearKey();
        return true;
    }

    [[nodiscard]] bool scalar(std::string_view value) noexcept
    {
        if (m_complete || m_frames.empty())
            return false;

        Frame &frame = m_frames.back();

        if (frame.context == Context::Sequence)
            return frame.sequenceScalarReader && frame.sequenceScalarReader(frame.destination, value);

        if (!frame.hasKey || !frame.scalarReader)
            return false;

        if (!frame.scalarReader(frame.destination, frame.key(), value))
            return false;

        frame.clearKey();
        return true;
    }

    [[nodiscard]] bool scalarOwned(std::string value) noexcept
    {
        return scalar(value);
    }

    [[nodiscard]] bool key(std::string_view key) noexcept
    {
        if (m_complete || m_frames.empty())
            return false;

        Frame &frame = m_frames.back();

        if (frame.context != Context::Object || frame.hasKey)
            return false;

        frame.keyView = key;
        frame.keyStorage.clear();
        frame.keyOwned = false;
        frame.hasKey = true;
        return true;
    }

    [[nodiscard]] bool keyOwned(std::string key) noexcept
    {
        if (m_complete || m_frames.empty())
            return false;

        Frame &frame = m_frames.back();

        if (frame.context != Context::Object || frame.hasKey)
            return false;

        frame.keyView = {};
        frame.keyStorage = std::move(key);
        frame.keyOwned = true;
        frame.hasKey = true;
        return true;
    }

    [[nodiscard]] bool beginMapping() noexcept
    {
        if (m_complete || m_frames.size() == m_frames.capacity())
            return false;

        if (m_frames.empty()) {
            if constexpr (nestedObject<T>()) {
                m_frames.push_back(makeObjectFrame(*m_destination));
                return true;
            }

            return false;
        }

        Frame &parent = m_frames.back();
        Frame child{};

        if (parent.context == Context::Object) {
            if (!parent.hasKey || !parent.mappingReader)
                return false;

            if (!parent.mappingReader(parent.destination, parent.key(), child))
                return false;

            parent.clearKey();
        } else {
            if (!parent.sequenceMappingReader)
                return false;

            if (!parent.sequenceMappingReader(parent.destination, child))
                return false;
        }

        m_frames.push_back(std::move(child));
        return true;
    }

    [[nodiscard]] bool endMapping() noexcept
    {
        if (m_complete || m_frames.empty())
            return false;

        const Frame &frame = m_frames.back();

        if (frame.context != Context::Object || frame.hasKey)
            return false;

        m_frames.pop_back();

        if (m_frames.empty())
            m_complete = true;

        return true;
    }

    [[nodiscard]] bool beginSequence() noexcept
    {
        if (m_complete || m_frames.size() == m_frames.capacity())
            return false;

        if (m_frames.empty()) {
            if constexpr (YamlSequence<T>) {
                m_frames.push_back(makeSequenceFrame(*m_destination));
                return true;
            }

            return false;
        }

        Frame &parent = m_frames.back();
        Frame child{};

        if (parent.context == Context::Object) {
            if (!parent.hasKey || !parent.sequenceReader)
                return false;

            if (!parent.sequenceReader(parent.destination, parent.key(), child))
                return false;

            parent.clearKey();
        } else {
            if (!parent.sequenceSequenceReader)
                return false;

            if (!parent.sequenceSequenceReader(parent.destination, child))
                return false;
        }

        m_frames.push_back(std::move(child));
        return true;
    }

    [[nodiscard]] bool endSequence() noexcept
    {
        if (m_complete || m_frames.empty())
            return false;

        if (m_frames.back().context != Context::Sequence)
            return false;

        m_frames.pop_back();

        if (m_frames.empty())
            m_complete = true;

        return true;
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
        Object,
        Sequence
    };

    struct Frame;

    using NullReader = bool (*)(void *, std::string_view) noexcept;
    using ScalarReader = bool (*)(void *, std::string_view, std::string_view) noexcept;
    using MappingReader = bool (*)(void *, std::string_view, Frame &) noexcept;
    using SequenceReader = bool (*)(void *, std::string_view, Frame &) noexcept;

    using SequenceNullReader = bool (*)(void *) noexcept;
    using SequenceScalarReader = bool (*)(void *, std::string_view) noexcept;
    using SequenceMappingReader = bool (*)(void *, Frame &) noexcept;
    using SequenceSequenceReader = bool (*)(void *, Frame &) noexcept;

    struct Frame
    {
        Context context{Context::Object};
        void *destination{};

        NullReader nullReader{};
        ScalarReader scalarReader{};
        MappingReader mappingReader{};
        SequenceReader sequenceReader{};

        SequenceNullReader sequenceNullReader{};
        SequenceScalarReader sequenceScalarReader{};
        SequenceMappingReader sequenceMappingReader{};
        SequenceSequenceReader sequenceSequenceReader{};

        std::string_view keyView{};
        std::string keyStorage{};
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

    template <typename Object>
    [[nodiscard]] static Frame makeObjectFrame(Object &object) noexcept
    {
        using ObjectType = YamlType<Object>;

        return Frame{
            .context = Context::Object,
            .destination = &object,
            .nullReader = &readNull<ObjectType>,
            .scalarReader = &readScalar<ObjectType>,
            .mappingReader = &readMapping<ObjectType>,
            .sequenceReader = &readSequence<ObjectType>,
            .sequenceNullReader = nullptr,
            .sequenceScalarReader = nullptr,
            .sequenceMappingReader = nullptr,
            .sequenceSequenceReader = nullptr,
            .keyView = {},
            .keyStorage = {},
            .keyOwned = false,
            .hasKey = false
        };
    }

    template <typename Sequence>
    [[nodiscard]] static consteval SequenceNullReader sequenceNullReader() noexcept
    {
        using SequenceType = YamlType<Sequence>;
        using ElementType = std::ranges::range_value_t<SequenceType>;

        if constexpr (YamlOptional<ElementType> || YamlPointer<ElementType>)
            return &appendNull<SequenceType>;
        else
            return nullptr;
    }

    template <typename Sequence>
    [[nodiscard]] static consteval SequenceScalarReader sequenceScalarReader() noexcept
    {
        using SequenceType = YamlType<Sequence>;
        using ElementType = std::ranges::range_value_t<SequenceType>;

        if constexpr (YamlScalar<ElementType>)
            return &appendScalar<SequenceType>;
        else
            return nullptr;
    }

    template <typename Sequence>
    [[nodiscard]] static consteval SequenceMappingReader sequenceMappingReader() noexcept
    {
        using SequenceType = YamlType<Sequence>;
        using ElementType = std::ranges::range_value_t<SequenceType>;

        if constexpr (nestedObject<ElementType>())
            return &appendMapping<SequenceType>;
        else
            return nullptr;
    }

    template <typename Sequence>
    [[nodiscard]] static consteval SequenceSequenceReader sequenceSequenceReader() noexcept
    {
        using SequenceType = YamlType<Sequence>;
        using ElementType = std::ranges::range_value_t<SequenceType>;

        if constexpr (YamlSequence<ElementType>)
            return &appendSequence<SequenceType>;
        else
            return nullptr;
    }

    template <typename Sequence>
    [[nodiscard]] static Frame makeSequenceFrame(Sequence &sequence) noexcept
    {
        using SequenceType = YamlType<Sequence>;

        static_assert(YamlSequence<SequenceType>);

        return Frame{
            .context = Context::Sequence,
            .destination = &sequence,
            .nullReader = nullptr,
            .scalarReader = nullptr,
            .mappingReader = nullptr,
            .sequenceReader = nullptr,
            .sequenceNullReader = sequenceNullReader<SequenceType>(),
            .sequenceScalarReader = sequenceScalarReader<SequenceType>(),
            .sequenceMappingReader = sequenceMappingReader<SequenceType>(),
            .sequenceSequenceReader = sequenceSequenceReader<SequenceType>(),
            .keyView = {},
            .keyStorage = {},
            .keyOwned = false,
            .hasKey = false
        };
    }

    template <typename Object>
    [[nodiscard]] static bool readNull(void *object, std::string_view key) noexcept
    {
        bool resolved = false;

        const bool matched = YamlKeyDispatch::dispatch<Object>(key, [&]<auto member> {
            auto &destination = static_cast<Object *>(object)->[:member:];
            using MemberType = YamlType<decltype(destination)>;

            if constexpr (YamlOptional<MemberType> || YamlPointer<MemberType>) {
                destination.reset();
                resolved = true;
            }
        });

        return matched && resolved;
    }

    template <typename Object>
    [[nodiscard]] static bool readScalar(void *object, std::string_view key, std::string_view value) noexcept
    {
        return YamlObjectReader::readScalar(*static_cast<Object *>(object), key, value);
    }

    template <typename Object>
    [[nodiscard]] static bool readMapping(void *object, std::string_view key, Frame &frame) noexcept
    {
        bool resolved = false;

        const bool matched = YamlKeyDispatch::dispatch<Object>(key, [&]<auto member> {
            auto &destination = static_cast<Object *>(object)->[:member:];
            using MemberType = YamlType<decltype(destination)>;

            if constexpr (nestedObject<MemberType>()) {
                frame = makeObjectFrame(destination);
                resolved = true;
            }
        });

        return matched && resolved;
    }

    template <typename Object>
    [[nodiscard]] static bool readSequence(void *object, std::string_view key, Frame &frame) noexcept
    {
        bool resolved = false;

        const bool matched = YamlKeyDispatch::dispatch<Object>(key, [&]<auto member> {
            auto &destination = static_cast<Object *>(object)->[:member:];
            using MemberType = YamlType<decltype(destination)>;

            if constexpr (YamlSequence<MemberType>) {
                frame = makeSequenceFrame(destination);
                resolved = true;
            }
        });

        return matched && resolved;
    }

    template <typename Sequence>
    [[nodiscard]] static bool appendNull(void *sequence) noexcept
    {
        using SequenceType = YamlType<Sequence>;
        using ElementType = std::ranges::range_value_t<SequenceType>;

        auto &destination = *static_cast<SequenceType *>(sequence);

        if constexpr (YamlOptional<ElementType> || YamlPointer<ElementType>)
            return appendValue(destination, ElementType{});
        else
            return false;
    }

    template <typename Sequence>
    [[nodiscard]] static bool appendScalar(void *sequence, std::string_view value) noexcept
    {
        using SequenceType = YamlType<Sequence>;
        using ElementType = std::ranges::range_value_t<SequenceType>;

        auto &destination = *static_cast<SequenceType *>(sequence);

        if constexpr (YamlOwnedString<ElementType>) {
            return appendValue(destination, std::string{value});
        } else if constexpr (YamlStringView<ElementType>) {
            return appendValue(destination, value);
        } else if constexpr (YamlScalar<ElementType>) {
            ElementType element{};

            if (!YamlScalarKernel::parse(value, element))
                return false;

            return appendValue(destination, std::move(element));
        } else {
            return false;
        }
    }

    template <typename Sequence>
    [[nodiscard]] static bool appendMapping(void *sequence, Frame &frame) noexcept
    {
        using SequenceType = YamlType<Sequence>;
        using ElementType = std::ranges::range_value_t<SequenceType>;

        if constexpr (nestedObject<ElementType>()) {
            auto &destination = *static_cast<SequenceType *>(sequence);
            ElementType *element = appendDefault(destination);

            if (!element)
                return false;

            frame = makeObjectFrame(*element);
            return true;
        } else {
            return false;
        }
    }

    template <typename Sequence>
    [[nodiscard]] static bool appendSequence(void *sequence, Frame &frame) noexcept
    {
        using SequenceType = YamlType<Sequence>;
        using ElementType = std::ranges::range_value_t<SequenceType>;

        if constexpr (YamlSequence<ElementType>) {
            auto &destination = *static_cast<SequenceType *>(sequence);
            ElementType *element = appendDefault(destination);

            if (!element)
                return false;

            frame = makeSequenceFrame(*element);
            return true;
        } else {
            return false;
        }
    }

    template <typename Sequence, typename Value>
    [[nodiscard]] static bool appendValue(Sequence &sequence, Value &&value) noexcept
    {
        if constexpr (requires {
                          sequence.emplace_back(std::forward<Value>(value));
                      }) {
            sequence.emplace_back(std::forward<Value>(value));
            return true;
        } else if constexpr (requires {
                                 sequence.push_back(std::forward<Value>(value));
                             }) {
            sequence.push_back(std::forward<Value>(value));
            return true;
        } else {
            return false;
        }
    }

    template <typename Sequence>
    [[nodiscard]] static auto appendDefault(Sequence &sequence) noexcept
        -> std::ranges::range_value_t<Sequence> *
    {
        using ElementType = std::ranges::range_value_t<Sequence>;

        if constexpr (std::default_initializable<ElementType> &&
                      requires {
                          { sequence.emplace_back() } -> std::same_as<ElementType &>;
                      }) {
            return &sequence.emplace_back();
        } else if constexpr (std::default_initializable<ElementType> &&
                             requires {
                                 sequence.push_back(ElementType{});
                                 sequence.back();
                             }) {
            sequence.push_back(ElementType{});
            return &sequence.back();
        } else {
            return nullptr;
        }
    }

    template <typename Object>
    [[nodiscard]] static consteval bool nestedObject() noexcept
    {
        using ObjectType = YamlType<Object>;

        return std::is_class_v<ObjectType> &&
               !YamlScalar<ObjectType> &&
               !YamlOptional<ObjectType> &&
               !YamlPointer<ObjectType> &&
               !YamlContainer<ObjectType>;
    }

    T *m_destination{};
    Frames m_frames;
    bool m_complete{};
};

} // namespace job::yaml