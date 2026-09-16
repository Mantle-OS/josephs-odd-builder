#pragma once

#include <utility>
#include "job_obj_grammar.h"

// ----------------------------------------
// Shared Pointer
// ----------------------------------------
#define JPTR_RW(type, name) \
private: \
    ::job::core::Ptr<type> m_##name; \
public: \
    [[nodiscard]] constexpr const ::job::core::Ptr<type> & get_##name ( void ) const noexcept { return m_##name; } \
    void set_##name ( const ::job::core::Ptr<type> & name ) noexcept pre ( name != nullptr ) post ( m_##name == name ) { if ( m_##name != name )  m_##name = name; } \
    template <typename... Args> \
    void init_##name ( Args &&... args ) { m_##name = type::createShared ( std::forward<Args> ( args )... ); } \
private:

#define JPTR_RO(type, name) \
private: \
    ::job::core::Ptr<type> m_##name; \
public: \
    [[nodiscard]] constexpr const ::job::core::Ptr<type> & get_##name ( void ) const noexcept { return m_##name; } \
protected: \
    void set_##name ( const ::job::core::Ptr<type> & name ) noexcept pre ( name != nullptr ) post ( m_##name == name ) { if ( m_##name != name ) m_##name = name; } \
    template <typename... Args> \
    void init_##name ( Args &&... args ) { m_##name = type::createShared ( std::forward<Args> ( args )... ); } \
private:

#define JPTR_CONST(type, name) \
private: \
    ::job::core::Ptr<type> m_##name; \
public: \
    [[nodiscard]] constexpr const ::job::core::Ptr<type> & get_##name ( void ) const noexcept { return m_##name; } \
protected: \
    template <typename... Args> \
    void init_##name ( Args &&... args ) { m_##name = type::createShared ( std::forward<Args> ( args )... ); } \
private:

// ----------------------------------------
// Weak Pointer
// ----------------------------------------

#define JWPTR_RW(type, name) \
private: \
    ::job::core::WPtr<type> m_##name; \
public: \
    [[nodiscard]] constexpr const ::job::core::WPtr<type> & get_##name ( void ) const noexcept { return m_##name; } \
    void set_##name ( const ::job::core::WPtr<type> & name ) noexcept { m_##name = name; } \
private:

#define JWPTR_RO(type, name) \
private: \
    ::job::core::WPtr<type> m_##name; \
public: \
    [[nodiscard]] constexpr const ::job::core::WPtr<type> & get_##name ( void ) const noexcept { return m_##name; } \
protected: \
    void set_##name ( const ::job::core::WPtr<type> & name ) noexcept { m_##name = name; } \
private:

#define JWPTR_CONST(type, name) \
private: \
    ::job::core::WPtr<type> m_##name; \
public: \
    [[nodiscard]] constexpr const ::job::core::WPtr<type> & get_##name ( void ) const noexcept { return m_##name; } \
private:

// ----------------------------------------
// Unique Pointer
// ----------------------------------------
#define JUPTR_RW(type, name) \
private: \
    ::job::core::UPtr<type> m_##name; \
public: \
    [[nodiscard]] constexpr const ::job::core::UPtr<type> & get_##name ( void ) const noexcept { return m_##name; } \
    void set_##name ( ::job::core::UPtr<type> name ) noexcept { m_##name = std::move ( name ); } \
    template <typename... Args> \
    void init_##name ( Args &&... args ) { m_##name = type::createUniq ( std::forward<Args> ( args )... ); } \
private:

#define JUPTR_RO(type, name) \
private: \
    ::job::core::UPtr<type> m_##name; \
public: \
    [[nodiscard]] constexpr const ::job::core::UPtr<type> & get_##name ( void ) const noexcept { return m_##name; } \
protected: \
    void set_##name ( ::job::core::UPtr<type> name ) noexcept pre ( name != nullptr ) post ( m_##name != nullptr ) { m_##name = std::move ( name ); } \
    template <typename... Args> \
    void init_##name ( Args &&... args ) { m_##name = type::createUniq ( std::forward<Args> ( args )... ); } \
private:

#define JUPTR_CONST(type, name) \
private: \
    ::job::core::UPtr<type> m_##name; \
public: \
    [[nodiscard]] constexpr const ::job::core::UPtr<type> & get_##name ( void ) const noexcept { return m_##name; } \
protected: \
    template <typename... Args> \
    void init_##name ( Args &&... args ) { m_##name = type::createUniq ( std::forward<Args> ( args )... ); } \
private:
