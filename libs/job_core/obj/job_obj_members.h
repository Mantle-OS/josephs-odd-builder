#pragma once

#include "job_obj_annotation.h"
#include "job_signal.h"

// ----------------------------------------
// Read / Write
// ----------------------------------------
#define JRW(type, name, defaultValue) \
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
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
private:

#define JRW_EMIT(type, name, defaultValue) \
private: \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
public: \
    ::job::core::Signal < const type & > name##Changed; \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
    void set_##name ( const type & name ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
            name##Changed.emit ( m_##name ); \
        } \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
private:

#define JRW_REQ(type, name, defaultValue) \
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
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
private:

#define JRW_EMIT_REQ(type, name, defaultValue) \
private: \
    [[=::job::core::Required{}]] \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
public: \
    ::job::core::Signal < const type & > name##Changed; \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
    void set_##name ( const type & name ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
            name##Changed.emit ( m_##name ); \
        } \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
private:

// ----------------------------------------
// Read Only
// ----------------------------------------
#define JRO(type, name, defaultValue) \
private: \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
protected: \
    constexpr void set_##name ( const type & name ) noexcept ( noexcept ( m_##name != name ) && noexcept ( m_##name = name ) ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
        } \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
private:

#define JRO_EMIT(type, name, defaultValue) \
private: \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
public: \
    ::job::core::Signal < const type & > name##Changed; \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
protected: \
    void set_##name ( const type & name ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
            name##Changed.emit ( m_##name ); \
        } \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
private:

#define JRO_REQ(type, name, defaultValue) \
private: \
    [[=::job::core::Required{}]] \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
protected: \
    constexpr void set_##name ( const type & name ) noexcept ( noexcept ( m_##name != name ) && noexcept ( m_##name = name ) ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
        } \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
private:

#define JRO_EMIT_REQ(type, name, defaultValue) \
private: \
    [[=::job::core::Required{}]] \
    type m_##name = default_##name ( ); \
    [[nodiscard]] static const type & default_##name ( void ) { \
        static const type value = defaultValue; \
        return value; \
    } \
public: \
    ::job::core::Signal < const type & > name##Changed; \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
protected: \
    void set_##name ( const type & name ) { \
        if ( m_##name != name ) { \
            m_##name = name; \
            name##Changed.emit ( m_##name ); \
        } \
    } \
    void reset_##name ( void ) { set_##name ( default_##name ( ) ); } \
private:

// ----------------------------------------
// Const
// ----------------------------------------

#define JCONST(type, name, defaultValue) \
private: \
    type m_##name = defaultValue; \
public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
private:

#define JCONST_REQ(type, name, defaultValue) \
private: \
    [[=::job::core::Required{}]] \
    type m_##name = defaultValue; \
public: \
    [[nodiscard]] constexpr const type & get_##name ( void ) const noexcept { return m_##name; } \
private:
