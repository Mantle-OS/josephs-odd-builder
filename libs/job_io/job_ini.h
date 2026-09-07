#pragma once

#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <job_base_obj.h>

namespace job::io {

template <typename T>
concept IniLocalizedStringType =
    requires(std::remove_cvref_t<T> value) {
        requires std::same_as<
            std::remove_cvref_t<decltype(value.value)>,
            std::string>;

        requires std::same_as<
            std::remove_cvref_t<decltype(value.localized)>,
            std::map<std::string, std::string>>;
    };

template <typename T>
concept IniStringListType =
    std::same_as<
        std::remove_cvref_t<T>,
        std::vector<std::string>>;

template <typename T>
concept IniGroupedObject =
    requires {
        { std::remove_cvref_t<T>::IniGroup } ->
            std::convertible_to<std::string_view>;
    };

template <typename T>
concept IniCustomDeserializer =
    requires(std::string_view source, T &value) {
        { fromIniValue(source, value) } -> std::same_as<bool>;
    };

template <typename T>
concept IniCustomSerializer =
    requires(const T &value) {
        { toIniValue(value) } -> std::convertible_to<std::string>;
    };

template <typename T>
consteval bool isIniValue()
{
    using V = std::remove_cvref_t<T>;

    if constexpr (core::OptionalType<V>) {
        return isIniValue<typename V::value_type>();
    } else {
        return IniCustomDeserializer<V> ||
               std::same_as<V, std::string> ||
               std::same_as<V, bool> ||
               std::is_enum_v<V> ||
               std::integral<V> ||
               std::floating_point<V> ||
               IniStringListType<V> ||
               IniLocalizedStringType<V>;
    }
}

template <typename T>
concept IniValue = isIniValue<T>();

class JobIni : public core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<JobIni>;
    using WPtr = std::weak_ptr<JobIni>;
    using UPtr = std::unique_ptr<JobIni>;

    JobIni() = default;
    ~JobIni() override = default;

    JobIni(const JobIni &) = default;
    JobIni &operator=(const JobIni &) = default;
    JobIni(JobIni &&) noexcept = default;
    JobIni &operator=(JobIni &&) noexcept = default;

    template <typename... Args>
        requires std::constructible_from<JobIni, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<JobIni>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<JobIni, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<JobIni>(std::forward<Args>(args)...);
    }

    //////////////////////////////////////////////////////////
    // INI serialization
    //////////////////////////////////////////////////////////

    template <typename Self>
    [[nodiscard]] std::string toIni(this const Self &self)
    {
        std::string result;

        if constexpr (IniGroupedObject<Self>) {
            result.push_back(Grammar::GroupOpen);
            result.append(std::string_view{Self::IniGroup});
            result.push_back(Grammar::GroupClose);
            result.push_back(Grammar::NewLine);
        }

        template for (constexpr auto member : core::reflectedDataMembersV<Self>) {
            using MemberType = typename[:std::meta::type_of(member):];

            if constexpr (!core::hasNoSerializeAnnotation(member) &&
                          IniValue<MemberType>) {
                constexpr std::string_view name = std::meta::identifier_of(member);
                serializeIniValue(result, name, self.[:member:]);
            }
        }

        return result;
    }

    template <typename Self>
    [[nodiscard]] bool fromIni(this Self &self, std::string_view source)
    {
        try {
            Cursor cursor{source};
            std::string_view group;
            std::string_view line;

            while (nextLineView(cursor, line)) {
                line = trimView(line);

                switch (classify(line)) {
                case TokenType::Empty:
                case TokenType::Comment:
                    continue;

                case TokenType::Group:
                    if (!groupView(line, group)) {
                        self.lastErrorString = "Invalid INI group";
                        return false;
                    }
                    continue;

                case TokenType::Entry: {
                    std::string_view key;
                    std::string_view value;

                    if (!entryView(line, key, value)) {
                        self.lastErrorString = "Invalid INI entry";
                        return false;
                    }

                    if (!deserializeIniEntry(self, group, key, value))
                        return false;

                    break;
                }
                }
            }

            return true;
        } catch (const std::exception &e) {
            self.lastErrorString = e.what();
            return false;
        }
    }

    template <typename Self>
    void debugIni(this const Self &self)
    {
        std::cout << "[INI]\n" << self.toIni() << std::endl;
    }

    //////////////////////////////////////////////////////////
    // INI files
    //////////////////////////////////////////////////////////

    template <typename Self>
    [[nodiscard]] bool saveToIniFile(this const Self &self,
                                     const std::string &fileName)
    {
        std::ofstream file(fileName);

        if (!file.is_open()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        file << self.toIni();

        if (!file.good()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        return true;
    }

    template <typename Self>
    [[nodiscard]] bool loadFromIniFile(this Self &self,
                                       const std::string &fileName)
    {
        std::ifstream file(fileName, std::ios::binary);

        if (!file.is_open()) {
            self.lastErrorString = "Failed reading file: " + fileName;
            return false;
        }

        file.seekg(0, std::ios::end);
        const std::streampos end = file.tellg();

        if (end < 0) {
            self.lastErrorString = "Failed determining file size: " + fileName;
            return false;
        }

        std::string source(static_cast<std::size_t>(end), '\0');

        file.seekg(0, std::ios::beg);

        if (!source.empty())
            file.read(source.data(), static_cast<std::streamsize>(source.size()));

        if (!file.good() && !file.eof()) {
            self.lastErrorString = "Failed reading file: " + fileName;
            return false;
        }

        return self.fromIni(source);
    }

private:
    //////////////////////////////////////////////////////////
    // Grammar
    //////////////////////////////////////////////////////////

    struct Grammar final
    {
        static constexpr char CommentHash      = '#';
        static constexpr char CommentSemicolon = ';';

        static constexpr char GroupOpen  = '[';
        static constexpr char GroupClose = ']';

        static constexpr char LocaleOpen  = '[';
        static constexpr char LocaleClose = ']';

        static constexpr char Assignment = '=';

        static constexpr char ListSeparator = ';';
        static constexpr char Escape        = '\\';

        static constexpr char NewLine        = '\n';
        static constexpr char CarriageReturn = '\r';

        static constexpr char Space = ' ';
        static constexpr char Tab   = '\t';
    };

    enum class TokenType : std::uint8_t
    {
        Empty,
        Comment,
        Group,
        Entry
    };

    struct Cursor final
    {
        std::string_view source;
        std::size_t position{};
    };

    //////////////////////////////////////////////////////////
    // Lexer
    //////////////////////////////////////////////////////////

    [[nodiscard]] static bool nextLineView(Cursor &cursor,
                                           std::string_view &line) noexcept
    {
        if (cursor.position >= cursor.source.size())
            return false;

        const std::size_t newline =
            cursor.source.find(Grammar::NewLine, cursor.position);

        if (newline == std::string_view::npos) {
            line = cursor.source.subview(cursor.position);
            cursor.position = cursor.source.size();
        } else {
            line = cursor.source.subview(cursor.position,
                                         newline - cursor.position);
            cursor.position = newline + 1;
        }

        if (!line.empty() && line.back() == Grammar::CarriageReturn)
            line.remove_suffix(1);

        return true;
    }

    [[nodiscard]] static constexpr TokenType classify(std::string_view line) noexcept
    {
        if (line.empty())
            return TokenType::Empty;

        switch (line.front()) {
        case Grammar::CommentHash:
        case Grammar::CommentSemicolon:
            return TokenType::Comment;

        case Grammar::GroupOpen:
            return TokenType::Group;

        default:
            return TokenType::Entry;
        }
    }

    [[nodiscard]] static std::string_view trimView(std::string_view value) noexcept
    {
        while (!value.empty() &&
               (value.front() == Grammar::Space ||
                value.front() == Grammar::Tab)) {
            value.remove_prefix(1);
        }

        while (!value.empty() &&
               (value.back() == Grammar::Space ||
                value.back() == Grammar::Tab)) {
            value.remove_suffix(1);
        }

        return value;
    }

    [[nodiscard]] static bool groupView(std::string_view line,
                                        std::string_view &group) noexcept
    {
        if (line.size() < 3 ||
            line.front() != Grammar::GroupOpen ||
            line.back() != Grammar::GroupClose) {
            return false;
        }

        group = trimView(line.subview(1, line.size() - 2));

        if (group.empty() ||
            group.find(Grammar::GroupOpen) != std::string_view::npos ||
            group.find(Grammar::GroupClose) != std::string_view::npos) {
            return false;
        }

        return true;
    }

    [[nodiscard]] static bool entryView(std::string_view line,
                                        std::string_view &key,
                                        std::string_view &value) noexcept
    {
        const std::size_t assignment = line.find(Grammar::Assignment);

        if (assignment == std::string_view::npos)
            return false;

        key = trimView(line.subview(0, assignment));
        value = trimView(line.subview(assignment + 1));

        return !key.empty();
    }

    [[nodiscard]] static bool localizedKeyView(std::string_view key,
                                               std::string_view &base,
                                               std::string_view &locale) noexcept
    {
        base = key;
        locale = {};

        const std::size_t open = key.find(Grammar::LocaleOpen);

        if (open == std::string_view::npos)
            return true;

        if (open == 0 ||
            key.back() != Grammar::LocaleClose ||
            key.find(Grammar::LocaleOpen, open + 1) != std::string_view::npos) {
            return false;
        }

        base = key.subview(0, open);
        locale = key.subview(open + 1, key.size() - open - 2);

        return !base.empty() && !locale.empty();
    }

    //////////////////////////////////////////////////////////
    // Reflection dispatch
    //////////////////////////////////////////////////////////

    template <typename Self>
    [[nodiscard]] static bool deserializeIniEntry(Self &self,
                                                  std::string_view group,
                                                  std::string_view key,
                                                  std::string_view value)
    {
        if constexpr (IniGroupedObject<Self>) {
            if (group != std::string_view{Self::IniGroup})
                return true;
        }

        std::string_view baseKey;
        std::string_view locale;

        if (!localizedKeyView(key, baseKey, locale)) {
            self.lastErrorString = "Invalid localized INI key: " + std::string{key};
            return false;
        }

        bool matched = false;
        bool success = true;

        template for (constexpr auto member : core::reflectedDataMembersV<Self>) {
            using MemberType = typename[:std::meta::type_of(member):];

            if constexpr (!core::hasNoSerializeAnnotation(member) &&
                          IniValue<MemberType>) {
                constexpr std::string_view name = std::meta::identifier_of(member);

                if constexpr (IniLocalizedStringType<MemberType>) {
                    if (!matched && baseKey == name) {
                        matched = true;

                        if (locale.empty())
                            self.[:member:].value.assign(value);
                        else
                            self.[:member:].localized[std::string{locale}].assign(value);
                    }
                } else {
                    if (!matched && locale.empty() && key == name) {
                        matched = true;
                        success = deserializeIniValue(value, self.[:member:]);
                    }
                }
            }
        }

        if (!matched)
            return true;

        if (!success) {
            self.lastErrorString = "Failed parsing INI value for key: " + std::string{key};
            return false;
        }

        return true;
    }

    //////////////////////////////////////////////////////////
    // Value deserialization
    //////////////////////////////////////////////////////////

    template <typename T>
    [[nodiscard]] static bool deserializeIniValue(std::string_view value,
                                                  T &destination)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (IniCustomDeserializer<V>) {
            return fromIniValue(value, destination);
        } else if constexpr (std::same_as<V, std::string>) {
            destination.assign(value);
            return true;
        } else if constexpr (std::same_as<V, bool>) {
            if (value == "true") {
                destination = true;
                return true;
            }

            if (value == "false") {
                destination = false;
                return true;
            }

            return false;
        } else if constexpr (core::OptionalType<V>) {
            typename V::value_type parsed{};

            if (!deserializeIniValue(value, parsed))
                return false;

            destination = std::move(parsed);
            return true;
        } else if constexpr (IniStringListType<V>) {
            return deserializeIniStringList(value, destination);
        } else if constexpr (std::is_enum_v<V>) {
            std::underlying_type_t<V> underlying{};

            if (!deserializeIniValue(value, underlying))
                return false;

            destination = static_cast<V>(underlying);
            return true;
        } else if constexpr (std::integral<V>) {
            V parsed{};

            const auto [ptr, ec] =
                std::from_chars(value.data(),
                                value.data() + value.size(),
                                parsed);

            if (ec != std::errc{} ||
                ptr != value.data() + value.size()) {
                return false;
            }

            destination = parsed;
            return true;
        } else if constexpr (std::floating_point<V>) {
            V parsed{};

            const auto [ptr, ec] =
                std::from_chars(value.data(),
                                value.data() + value.size(),
                                parsed);

            if (ec != std::errc{} ||
                ptr != value.data() + value.size()) {
                return false;
            }

            destination = parsed;
            return true;
        } else {
            static_assert(core::dependentFalseV<V>,
                          "Unsupported type in INI deserialization");
        }
    }

    [[nodiscard]] static bool deserializeIniStringList(
        std::string_view value,
        std::vector<std::string> &destination)
    {
        destination.clear();

        std::string current;
        current.reserve(value.size());

        bool escaped = false;
        bool endedWithSeparator = false;

        for (const char c : value) {
            if (escaped) {
                switch (c) {
                case 's':
                    current.push_back(Grammar::Space);
                    break;

                case 'n':
                    current.push_back(Grammar::NewLine);
                    break;

                case 't':
                    current.push_back(Grammar::Tab);
                    break;

                case 'r':
                    current.push_back(Grammar::CarriageReturn);
                    break;

                case Grammar::Escape:
                    current.push_back(Grammar::Escape);
                    break;

                case Grammar::ListSeparator:
                    current.push_back(Grammar::ListSeparator);
                    break;

                default:
                    current.push_back(Grammar::Escape);
                    current.push_back(c);
                    break;
                }

                escaped = false;
                endedWithSeparator = false;
                continue;
            }

            if (c == Grammar::Escape) {
                escaped = true;
                continue;
            }

            if (c == Grammar::ListSeparator) {
                destination.push_back(std::move(current));
                current.clear();
                endedWithSeparator = true;
                continue;
            }

            current.push_back(c);
            endedWithSeparator = false;
        }

        if (escaped)
            current.push_back(Grammar::Escape);

        if (!endedWithSeparator)
            destination.push_back(std::move(current));

        return true;
    }

    //////////////////////////////////////////////////////////
    // Value serialization
    //////////////////////////////////////////////////////////

    template <typename T>
    static void serializeIniValue(std::string &result,
                                  std::string_view key,
                                  const T &value)
    {
        using V = std::remove_cvref_t<T>;

        if constexpr (IniCustomSerializer<V>) {
            appendIniEntry(result, key, toIniValue(value));
        } else if constexpr (std::same_as<V, std::string>) {
            appendIniEntry(result, key, value);
        } else if constexpr (std::same_as<V, bool>) {
            appendIniEntry(result, key, value ? "true" : "false");
        } else if constexpr (core::OptionalType<V>) {
            if (value)
                serializeIniValue(result, key, *value);
        } else if constexpr (IniStringListType<V>) {
            serializeIniStringList(result, key, value);
        } else if constexpr (IniLocalizedStringType<V>) {
            serializeIniLocalizedString(result, key, value);
        } else if constexpr (std::is_enum_v<V>) {
            serializeIniValue(result, key,
                              static_cast<std::underlying_type_t<V>>(value));
        } else if constexpr (std::integral<V>) {
            char buffer[32];

            const auto [ptr, ec] =
                std::to_chars(buffer, buffer + sizeof(buffer), value);

            if (ec == std::errc{}) {
                appendIniEntry(
                    result,
                    key,
                    std::string_view{
                                     buffer,
                                     static_cast<std::size_t>(ptr - buffer)});
            }
        } else if constexpr (std::floating_point<V>) {
            char buffer[64];

            const auto [ptr, ec] =
                std::to_chars(buffer, buffer + sizeof(buffer), value);

            if (ec == std::errc{}) {
                appendIniEntry(
                    result,
                    key,
                    std::string_view{
                                     buffer,
                                     static_cast<std::size_t>(ptr - buffer)});
            }
        } else {
            static_assert(core::dependentFalseV<V>,
                          "Unsupported type in INI serialization");
        }
    }

    static void serializeIniStringList(
        std::string &result,
        std::string_view key,
        const std::vector<std::string> &value)
    {
        result.append(key);
        result.push_back(Grammar::Assignment);

        for (const auto &item : value) {
            appendEscapedIniString(result, item);
            result.push_back(Grammar::ListSeparator);
        }

        result.push_back(Grammar::NewLine);
    }

    template <IniLocalizedStringType T>
    static void serializeIniLocalizedString(std::string &result,
                                            std::string_view key,
                                            const T &value)
    {
        appendIniEntry(result, key, value.value);

        for (const auto &[locale, localizedValue] : value.localized) {
            result.append(key);
            result.push_back(Grammar::LocaleOpen);
            result.append(locale);
            result.push_back(Grammar::LocaleClose);
            result.push_back(Grammar::Assignment);
            result.append(localizedValue);
            result.push_back(Grammar::NewLine);
        }
    }

    static void appendEscapedIniString(std::string &result,
                                       std::string_view value)
    {
        for (const char c : value) {
            switch (c) {
            case Grammar::Space:
                result.push_back(Grammar::Escape);
                result.push_back('s');
                break;

            case Grammar::NewLine:
                result.push_back(Grammar::Escape);
                result.push_back('n');
                break;

            case Grammar::Tab:
                result.push_back(Grammar::Escape);
                result.push_back('t');
                break;

            case Grammar::CarriageReturn:
                result.push_back(Grammar::Escape);
                result.push_back('r');
                break;

            case Grammar::Escape:
                result.push_back(Grammar::Escape);
                result.push_back(Grammar::Escape);
                break;

            case Grammar::ListSeparator:
                result.push_back(Grammar::Escape);
                result.push_back(Grammar::ListSeparator);
                break;

            default:
                result.push_back(c);
                break;
            }
        }
    }

    static void appendIniEntry(std::string &result,
                               std::string_view key,
                               std::string_view value)
    {
        result.append(key);
        result.push_back(Grammar::Assignment);
        result.append(value);
        result.push_back(Grammar::NewLine);
    }
};

} // namespace job::io