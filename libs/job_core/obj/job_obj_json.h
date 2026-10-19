#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "job_obj_annotation.h"
#include "job_json.h"

// ----------------------------------------
// Read / Write
// ----------------------------------------
#define JJSONRW(type, name, defaultValue) \
private: \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
    public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
    constexpr void set_##name ( const type & name ) noexcept ( noexcept ( m_##name != name ) && noexcept ( m_##name = name ) ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
        } \
    } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool read_##name ( T &value ) const { \
        return ::job::json::JobJson::fromJson ( m_##name, value ); \
    } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool write_##name ( const T &value ) { \
        type output; \
        if ( !::job::json::JobJson::toJson ( value, output ) ) return false; \
        set_##name ( output ); \
        return true; \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
    private:

#define JJSONRW_REQ(type, name, defaultValue) \
private: \
    [[=::job::core::Required{}]] \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
    public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
    constexpr void set_##name ( const type & name ) noexcept ( noexcept ( m_##name != name ) && noexcept ( m_##name = name ) ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
        } \
    } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool read_##name ( T &value ) const { \
        return ::job::json::JobJson::fromJson ( m_##name, value ); \
    } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool write_##name ( const T &value ) { \
        type output; \
        if ( !::job::json::JobJson::toJson ( value, output ) ) return false; \
        set_##name ( output ); \
        return true; \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
    private:

// ----------------------------------------
// Read Only
// ----------------------------------------
#define JJSONRO(type, name, defaultValue) \
private: \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
    public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool read_##name ( T &value ) const { \
        return ::job::json::JobJson::fromJson ( m_##name, value ); \
    } \
    protected: \
    constexpr void set_##name ( const type & name ) noexcept ( noexcept ( m_##name != name ) && noexcept ( m_##name = name ) ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
        } \
    } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool write_##name ( const T &value ) { \
        type output; \
        if ( !::job::json::JobJson::toJson ( value, output ) ) return false; \
        set_##name ( output ); \
        return true; \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
    private:

#define JJSONRO_REQ(type, name, defaultValue) \
private: \
    [[=::job::core::Required{}]] \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
    public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool read_##name ( T &value ) const { \
        return ::job::json::JobJson::fromJson ( m_##name, value ); \
    } \
    protected: \
    constexpr void set_##name ( const type & name ) noexcept ( noexcept ( m_##name != name ) && noexcept ( m_##name = name ) ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
        } \
    } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool write_##name ( const T &value ) { \
        type output; \
        if ( !::job::json::JobJson::toJson ( value, output ) ) return false; \
        set_##name ( output ); \
        return true; \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
    private:

// ----------------------------------------
// Const
// ----------------------------------------
#define JJSONCONST(type, name, defaultValue) \
private: \
    type m_##name = defaultValue; \
    public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool read_##name ( T &value ) const { \
        return ::job::json::JobJson::fromJson ( m_##name, value ); \
    } \
    private:

#define JJSONCONST_REQ(type, name, defaultValue) \
private: \
    [[=::job::core::Required{}]] \
    type m_##name = defaultValue; \
    public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
    template <::job::json::JsonValue T> \
    [[nodiscard]] bool read_##name ( T &value ) const { \
        return ::job::json::JobJson::fromJson ( m_##name, value ); \
    } \
    private:
