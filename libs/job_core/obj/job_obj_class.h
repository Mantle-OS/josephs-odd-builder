#pragma once

#include <memory>
#include <utility>

#include "job_base_obj.h"
#include "job_light_object.h"
#include "job_obj_concept.h"
#include "job_object.h"

// ----------------------------------------
// Base Object
// ----------------------------------------
#define JBaseObj(name, ns,  ...) \
namespace ns { \
class name : public ::job::core::BaseObject { \
public: \
    using Ptr  = std::shared_ptr< name >; \
    using WPtr = std::weak_ptr< name >; \
    using UPtr = std::unique_ptr< name >; \
    name ( ) = default; \
    ~name ( ) override = default; \
    name ( const name & ) = default; \
    name & operator= ( const name & ) = default; \
    name ( name && ) = default; \
    name & operator= ( name && ) = default; \
    template <typename... Args> \
    [[nodiscard]] static Ptr createShared ( Args &&... args ) { return std::make_shared< name > ( std::forward< Args > ( args )... ); } \
    template <typename... Args> \
    [[nodiscard]] static UPtr createUniq ( Args &&... args ) { return std::make_unique<name> ( std::forward<Args> ( args )... ); } \
    __VA_ARGS__ \
}; \
static_assert ( ::job::core::BaseObjectType < name > ); \
}

// ----------------------------------------
// Light Object
// ----------------------------------------

#define JLightObj(name, ns, ...) \
namespace ns { \
class name : public ::job::core::LightObject { \
public: \
    using Ptr  = std::shared_ptr<name>; \
    using WPtr = std::weak_ptr<name>; \
    using UPtr = std::unique_ptr<name>; \
    name ( ) = default; \
    ~name ( ) override = default; \
    name ( const name & ) = delete; \
    name & operator= ( const name & ) = delete; \
    name ( name && ) = delete; \
    name & operator= ( name && ) = delete; \
    template <typename... Args> \
    [[nodiscard]] static Ptr createShared ( Args &&... args ) { return std::make_shared<name> ( std::forward<Args> ( args )... ); } \
    template <typename... Args> \
    [[nodiscard]] static UPtr createUniq ( Args &&... args ) { return std::make_unique<name> ( std::forward<Args> ( args )... ); } \
    __VA_ARGS__ \
}; \
static_assert ( ::job::core::LightObjectType < name > ); \
}

// ----------------------------------------
// Object
// ----------------------------------------
#define JObj(name, ns, ...) \
namespace ns { \
class name : public ::job::core::Object { \
public: \
    using Ptr  = std::shared_ptr<name>; \
    using WPtr = std::weak_ptr<name>; \
    using UPtr = std::unique_ptr<name>; \
    name ( ) = default; \
    ~name ( ) override = default; \
    name ( const name & ) = delete; \
    name & operator= ( const name & ) = delete; \
    name ( name && ) = delete; \
    name & operator= ( name && ) = delete; \
    template <typename... Args> \
    [[nodiscard]] static Ptr createShared ( Args &&... args ) { return std::make_shared<name> ( std::forward<Args> ( args )... ); } \
    template <typename... Args> \
    [[nodiscard]] static UPtr createUniq ( Args &&... args ) { return std::make_unique<name> ( std::forward<Args> ( args )... ); } \
    __VA_ARGS__ \
}; \
static_assert ( ::job::core::ObjectType < name > ); \
}
