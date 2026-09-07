#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace job::yaml {

class YamlNode
{
public:
    enum class Type
    {
        Null,
        Scalar,
        Mapping,
        Sequence
    };

    enum class ScalarStorage
    {
        None,
        Owned,
        Borrowed
    };

    struct MappingEntry;

    using Mapping = std::vector<MappingEntry>;
    using Sequence = std::vector<YamlNode>;

    constexpr YamlNode() noexcept = default;

    explicit YamlNode(std::string_view value);

    ~YamlNode() = default;

    YamlNode(const YamlNode &) = default;
    YamlNode &operator=(const YamlNode &) = default;
    YamlNode(YamlNode &&) noexcept = default;
    YamlNode &operator=(YamlNode &&) noexcept = default;

    [[nodiscard]] constexpr Type type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr bool isNull() const noexcept
    {
        return m_type == Type::Null;
    }

    [[nodiscard]] constexpr bool isScalar() const noexcept
    {
        return m_type == Type::Scalar;
    }

    [[nodiscard]] constexpr bool isMapping() const noexcept
    {
        return m_type == Type::Mapping;
    }

    [[nodiscard]] constexpr bool isSequence() const noexcept
    {
        return m_type == Type::Sequence;
    }

    [[nodiscard]] constexpr ScalarStorage scalarStorage() const noexcept
    {
        return m_scalarStorage;
    }

    [[nodiscard]] constexpr bool ownsScalar() const noexcept
    {
        return m_scalarStorage == ScalarStorage::Owned;
    }

    [[nodiscard]] constexpr bool borrowsScalar() const noexcept
    {
        return m_scalarStorage == ScalarStorage::Borrowed;
    }

    [[nodiscard]] std::string_view scalar() const noexcept
    {
        if (!isScalar())
            return {};

        if (m_scalarStorage == ScalarStorage::Borrowed)
            return m_scalarView;

        return m_scalar;
    }

    [[nodiscard]] std::string &scalar()
    {
        materializeScalar();
        return m_scalar;
    }

    [[nodiscard]] const Mapping &mapping() const noexcept
    {
        return m_mapping;
    }

    [[nodiscard]] Mapping &mapping() noexcept
    {
        return m_mapping;
    }

    [[nodiscard]] const Sequence &sequence() const noexcept
    {
        return m_sequence;
    }

    [[nodiscard]] Sequence &sequence() noexcept
    {
        return m_sequence;
    }

    void setNull() noexcept
    {
        reset(Type::Null);
    }

    void setScalar(std::string_view value);

    void setScalarView(std::string_view value) noexcept;

    void setMapping() noexcept
    {
        reset(Type::Mapping);
    }

    void setSequence() noexcept
    {
        reset(Type::Sequence);
    }

    void setMember(std::string_view key, YamlNode value);

    void setMemberScalar(std::string_view key, std::string_view value);

    void setMemberScalarView(std::string_view key, std::string_view value);

    void append(YamlNode value);

    void appendScalar(std::string_view value);

    void appendScalarView(std::string_view value);

    void clear() noexcept
    {
        setNull();
    }

    [[nodiscard]] const YamlNode *member(std::string_view key) const noexcept;
    [[nodiscard]] YamlNode *member(std::string_view key) noexcept;

private:
    void reset(Type type) noexcept
    {
        m_type = type;
        m_scalarStorage = ScalarStorage::None;
        m_scalar.clear();
        m_scalarView = {};
        m_mapping.clear();
        m_sequence.clear();
    }

    void materializeScalar()
    {
        if (!isScalar()) {
            reset(Type::Scalar);
            m_scalarStorage = ScalarStorage::Owned;
            return;
        }

        if (m_scalarStorage == ScalarStorage::Borrowed) {
            m_scalar.assign(m_scalarView);
            m_scalarView = {};
            m_scalarStorage = ScalarStorage::Owned;
            return;
        }

        if (m_scalarStorage == ScalarStorage::None)
            m_scalarStorage = ScalarStorage::Owned;
    }

    void ensureMapping()
    {
        if (m_type == Type::Mapping)
            return;

        setMapping();
    }

    void ensureSequence()
    {
        if (m_type == Type::Sequence)
            return;

        setSequence();
    }

    Type m_type{Type::Null};
    ScalarStorage m_scalarStorage{ScalarStorage::None};

    std::string m_scalar{};
    std::string_view m_scalarView{};
    Mapping m_mapping{};
    Sequence m_sequence{};
};

struct YamlNode::MappingEntry
{
    std::string key{};
    YamlNode value{};
};

inline YamlNode::YamlNode(std::string_view value) :
    m_type(Type::Scalar),
    m_scalarStorage(ScalarStorage::Owned),
    m_scalar(value)
{
}

inline void YamlNode::setScalar(std::string_view value)
{
    reset(Type::Scalar);
    m_scalar.assign(value);
    m_scalarStorage = ScalarStorage::Owned;
}

inline void YamlNode::setScalarView(std::string_view value) noexcept
{
    reset(Type::Scalar);
    m_scalarView = value;
    m_scalarStorage = ScalarStorage::Borrowed;
}

inline void YamlNode::setMember(std::string_view key, YamlNode value)
{
    ensureMapping();

    m_mapping.push_back(MappingEntry{
        .key = std::string{key},
        .value = std::move(value)
    });
}

inline void YamlNode::setMemberScalar(std::string_view key, std::string_view value)
{
    setMember(key, YamlNode{value});
}

inline void YamlNode::setMemberScalarView(std::string_view key, std::string_view value)
{
    YamlNode node;
    node.setScalarView(value);
    setMember(key, std::move(node));
}

inline void YamlNode::append(YamlNode value)
{
    ensureSequence();
    m_sequence.push_back(std::move(value));
}

inline void YamlNode::appendScalar(std::string_view value)
{
    append(YamlNode{value});
}

inline void YamlNode::appendScalarView(std::string_view value)
{
    YamlNode node;
    node.setScalarView(value);
    append(std::move(node));
}

inline const YamlNode *YamlNode::member(std::string_view key) const noexcept
{
    if (!isMapping())
        return nullptr;

    for (const auto &entry : m_mapping) {
        if (entry.key == key)
            return &entry.value;
    }

    return nullptr;
}

inline YamlNode *YamlNode::member(std::string_view key) noexcept
{
    if (!isMapping())
        return nullptr;

    for (auto &entry : m_mapping) {
        if (entry.key == key)
            return &entry.value;
    }

    return nullptr;
}

} // namespace job::yaml