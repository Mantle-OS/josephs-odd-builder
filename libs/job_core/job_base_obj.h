#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <job_json.h>
#include <job_yaml.h>
#include <job_binary.h>

#include "jobcore_export.h"
#include "job_obj_annotation.h"
#include "job_obj_concept.h"

namespace job::core {

class JOBCORE_EXPORT BaseObject
{
public:
    using Ptr  = std::shared_ptr<BaseObject>;
    using WPtr = std::weak_ptr<BaseObject>;
    using UPtr = std::unique_ptr<BaseObject>;

    BaseObject();
    virtual ~BaseObject();

    BaseObject(const BaseObject &);
    BaseObject &operator=(const BaseObject &);
    BaseObject(BaseObject &&) noexcept;
    BaseObject &operator=(BaseObject &&) noexcept;

    [[=NoSerialize{}]]
        [[=NoReset{}]]
        std::string lastErrorString;


    // =========================================================================
    // JOB YAML Serialization
    // =========================================================================
    template <typename Self>
    bool toJobYaml(this const Self &self, std::string &yaml)
    {
        return job::yaml::JobYaml::toString(self, yaml);
    }

    template <typename Self>
    bool fromJobYaml(this Self &self, std::string_view yaml)
    {
        if (job::yaml::JobYaml::fromString(yaml, self))
            return true;

        self.lastErrorString = "Failed parsing JOB YAML";
        return false;
    }
    template <typename Self>
    bool saveToJobYamlFile(this const Self &self, const std::string &fileName)
    {
        std::string yaml;

        if (!self.toJobYaml(yaml)) {
            const_cast<Self &>(self).lastErrorString = "Failed serializing JOB YAML";
            return false;
        }

        std::ofstream file(fileName);

        if (!file.is_open()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        file << yaml;

        if (!file.good()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        return true;
    }

    template <typename Self>
    bool loadFromJobYamlFile(this Self &self, const std::string &fileName)
    {
        std::ifstream file(fileName);

        if (!file.is_open()) {
            self.lastErrorString = "Failed reading file: " + fileName;
            return false;
        }

        const std::string yaml{
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        };

        return self.fromJobYaml(yaml);
    }
    template <typename Self>
    void debugJobYaml(this const Self &self)
    {
        std::string yaml;

        if (self.toJobYaml(yaml))
            std::cout << "[JOB YAML]\n" << yaml << std::endl;
    }
    // =========================================================================
    // JOB JSON Serialization
    // =========================================================================

    template <typename Self>
    bool toJobJson(this const Self &self, std::string &json)
    {
        return job::json::JobJson::toJson(self, json);
    }

    template <typename Self>
    bool fromJobJson(this Self &self, std::string_view json)
    {
        job::json::JsonDiagnostic diagnostic;

        if (job::json::JobJson::fromJson(json, self, diagnostic))
            return true;

        self.lastErrorString = std::string{diagnostic.comment()};
        return false;
    }
    template <typename Self>
    bool saveToJobJsonFile(this const Self &self, const std::string &fileName)
    {
        std::string json;

        if (!self.toJobJson(json)) {
            const_cast<Self &>(self).lastErrorString = "Failed serializing JOB JSON";
            return false;
        }

        std::ofstream file(fileName);

        if (!file.is_open()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        file << json;

        if (!file.good()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing file: " + fileName;
            return false;
        }

        return true;
    }

    template <typename Self>
    bool loadFromJobJsonFile(this Self &self, const std::string &fileName)
    {
        std::ifstream file(fileName);

        if (!file.is_open()) {
            self.lastErrorString = "Failed reading file: " + fileName;
            return false;
        }

        const std::string json{
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        };

        return self.fromJobJson(json);
    }

    template <typename Self>
    void debugJobJson(this const Self &self)
    {
        std::string json;

        if (self.toJobJson(json))
            std::cout << "[JOB JSON]\n" << json << std::endl;
    }


    // =========================================================================
    // JOB Binary Serialization
    // =========================================================================

    template <typename Self>
    bool toBinary(this const Self &self, std::vector<std::uint8_t> &buffer)
    {
        return job::binary::JobBinary::toBytes(self, buffer);
    }

    template <typename Self>
    bool fromBinary(this Self &self, std::span<const std::uint8_t> streamSpan)
    {
        if (job::binary::JobBinary::fromBytes(streamSpan, self))
            return true;

        self.lastErrorString = "Failed parsing JOB binary data";
        return false;
    }
    template <typename Self>
    bool saveToBinaryFile(this const Self &self, const std::string &fileName)
    {
        std::vector<std::uint8_t> buffer;

        if (!self.toBinary(buffer)) {
            const_cast<Self &>(self).lastErrorString = "Failed serializing JOB binary";
            return false;
        }

        std::ofstream file(fileName, std::ios::binary);

        if (!file.is_open()) {
            const_cast<Self &>(self).lastErrorString = "Failed opening binary file: " + fileName;
            return false;
        }

        file.write(reinterpret_cast<const char *>(buffer.data()),
                   static_cast<std::streamsize>(buffer.size()));

        if (!file.good()) {
            const_cast<Self &>(self).lastErrorString = "Failed writing binary file: " + fileName;
            return false;
        }

        return true;
    }

    template <typename Self>
    bool loadFromBinaryFile(this Self &self, const std::string &fileName)
    {
        std::ifstream file(fileName, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            self.lastErrorString = "Failed opening binary file: " + fileName;
            return false;
        }

        const std::streamsize size = file.tellg();

        if (size < 0) {
            self.lastErrorString = "Failed determining binary file size: " + fileName;
            return false;
        }

        file.seekg(0, std::ios::beg);

        std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));

        if (size > 0 &&
            !file.read(reinterpret_cast<char *>(buffer.data()), size)) {
            self.lastErrorString = "Failed reading binary file stream";
            return false;
        }

        return self.fromBinary(std::span<const std::uint8_t>{buffer});
    }

private:
    template <OwningSmartPointer Pointer>
    static void constructPointer(Pointer &pointer)
    {
        using V = std::remove_cvref_t<Pointer>;
        using Elem = typename V::element_type;

        if constexpr (SharedPointer<V>) {
            if constexpr (requires { { Elem::createShared() } -> std::convertible_to<V>; })
                pointer = Elem::createShared();
            else
                pointer = std::make_shared<Elem>();
        } else if constexpr (UniquePointer<V>) {
            if constexpr (requires { { Elem::createUniq() } -> std::convertible_to<V>; }) {
                pointer = Elem::createUniq();
            } else if constexpr (std::constructible_from<V, Elem *>) {
                pointer = V(new Elem{});
            } else {
                static_assert(dependentFalseV<V>, "Unique pointer type cannot be reconstructed");
            }
        }
    }
};
} // namespace job::core