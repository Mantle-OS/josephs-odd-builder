#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "job_iana.h"
#include "jobnet_export.h"

namespace job::net {


class JOBNET_EXPORT JobHttpHeader {
public:
    struct Field {
        std::string name;           // normalized (lowercase) used for "all" lookups
        std::string displayName;    // as supplied by the caller / as received
        std::string value;          // OWS-trimmed
        [[nodiscard]] bool operator==(const Field &) const = default;
    };

    using FieldList = std::vector<Field>;

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    JobHttpHeader();
    JobHttpHeader(std::string_view name, std::string_view value);
    JobHttpHeader(const JobHttpHeader &other);
    JobHttpHeader(JobHttpHeader &&other) noexcept;
    ~JobHttpHeader();

    JobHttpHeader &operator=(const JobHttpHeader &other);
    JobHttpHeader &operator=(JobHttpHeader &&other) noexcept;

    [[nodiscard]] static std::string normalizeKey(std::string_view input);
    [[nodiscard]] static bool equalsIgnoreCase(std::string_view a, std::string_view b) noexcept;

    [[nodiscard]] static bool isValidFieldName(std::string_view name) noexcept;

    [[nodiscard]] static bool isValidFieldValue(std::string_view value) noexcept;

    [[nodiscard]] static bool isCombinable(std::string_view name) noexcept;

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] bool contains(std::string_view name) const noexcept;
    [[nodiscard]] bool contains(JobIana::IanaHeaders name) const noexcept;

    [[nodiscard]] std::size_t indexOf(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t lastIndexOf(std::string_view name) const noexcept;

    [[nodiscard]] std::size_t count(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t count(JobIana::IanaHeaders name) const noexcept;

    // Value of the FIRST field line with this name.
    // Joseph Note that when the name is repeated this is only part of the picture !!
    // use values() or joinedValue() when repetition is meaningful.
    [[nodiscard]] std::string_view value(std::string_view name, std::string_view defaultVal = {}) const noexcept;
    [[nodiscard]] std::string_view value(JobIana::IanaHeaders name, std::string_view defaultVal = {}) const noexcept;

    [[nodiscard]] std::string_view lastValue(std::string_view name, std::string_view defaultVal = {}) const noexcept;
    [[nodiscard]] std::string_view lastValue(JobIana::IanaHeaders name, std::string_view defaultVal = {}) const noexcept;


    [[nodiscard]] std::vector<std::string_view> values(std::string_view name) const;
    [[nodiscard]] std::vector<std::string_view> values(JobIana::IanaHeaders name) const;


    [[nodiscard]] std::string joinedValue(std::string_view name, std::string_view sep = ", ") const;
    [[nodiscard]] std::string joinedValue(JobIana::IanaHeaders name, std::string_view sep = ", ") const;

    [[nodiscard]] std::vector<std::string_view> listMembers(std::string_view name) const;
    [[nodiscard]] std::vector<std::string_view> listMembers(JobIana::IanaHeaders name) const;


    [[nodiscard]] const Field *fieldAt(std::size_t pos) const noexcept;
    [[nodiscard]] std::string_view nameAt(std::size_t pos) const noexcept;
    [[nodiscard]] std::string_view valueAt(std::size_t pos) const noexcept;

    // Adds a NEW field line, even if the name is already present.
    [[nodiscard]] bool append(std::string_view name, std::string_view value);
    [[nodiscard]] bool append(JobIana::IanaHeaders name, std::string_view value);

    // Adds a NEW field line at the front.
    [[nodiscard]] bool prepend(std::string_view name, std::string_view value);
    [[nodiscard]] bool prepend(JobIana::IanaHeaders name, std::string_view value);

    // Adds a NEW field line at pos (clamped to size()).
    [[nodiscard]] bool insert(std::string_view name, std::string_view value, std::size_t pos);
    [[nodiscard]] bool insert(JobIana::IanaHeaders name, std::string_view value, std::size_t pos);

    [[nodiscard]] bool set(std::string_view name, std::string_view value);
    [[nodiscard]] bool set(JobIana::IanaHeaders name, std::string_view value);

    // RFC 9110 5.3 list append: extends the LAST field line with this name
    // as "old, new", or creates the line if absent.
    // Returns false for non-combinable names (Set-Cookie) ->  use append().
    [[nodiscard]] bool appendToList(std::string_view name, std::string_view value);
    [[nodiscard]] bool appendToList(JobIana::IanaHeaders name, std::string_view value);

    // Overwrites the field line at pos.
    [[nodiscard]] bool replace(std::size_t pos, std::string_view name, std::string_view value);
    [[nodiscard]] bool replace(std::size_t pos, JobIana::IanaHeaders name, std::string_view value);

    bool removeAt(std::size_t pos);

    // Removes every field line with this name; returns how many were removed.
    std::size_t removeAll(std::string_view name);
    std::size_t removeAll(JobIana::IanaHeaders name);

    void clear() noexcept;
    void reserve(std::size_t n);

    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::size_t count() const noexcept; // compat

    [[nodiscard]] bool operator==(const JobHttpHeader &other) const
    {
        return m_fields == other.m_fields;
    }
    [[nodiscard]] bool operator!=(const JobHttpHeader &other) const
    {
        return !(*this == other);
    }

    [[nodiscard]] auto begin() noexcept         { return m_fields.begin();  }
    [[nodiscard]] auto end() noexcept           { return m_fields.end();    }
    [[nodiscard]] auto begin() const noexcept   { return m_fields.begin();  }
    [[nodiscard]] auto end() const noexcept     { return m_fields.end();    }
    [[nodiscard]] auto cbegin() const noexcept  { return m_fields.cbegin(); }
    [[nodiscard]] auto cend() const noexcept    { return m_fields.cend();   }

    // maybe private we will have to see.
    [[nodiscard]] static constexpr bool isTChar(unsigned char c) noexcept
    {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') ||
               c == '!' ||
               c == '#' ||
               c == '$' ||
               c == '%' ||
               c == '&' ||
               c == '\'' ||
               c == '*' ||
               c == '+' ||
               c == '-' ||
               c == '.' ||
               c == '^' ||
               c == '_' ||
               c == '`' ||
               c == '|' ||
               c == '~';
    }

    [[nodiscard]] static constexpr char lowerAscii(char c) noexcept
    {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }

private:
    [[nodiscard]] static std::string_view trimOws(std::string_view v) noexcept;
    [[nodiscard]] bool makeField(std::string_view name, std::string_view value, Field &out) const;

    FieldList m_fields;
};

} // namespace job::net
