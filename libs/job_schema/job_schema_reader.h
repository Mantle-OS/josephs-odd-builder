#pragma once

#include <memory>
#include <string>
#include <vector>

#include <io_factory.h>

#include "job_schema_concepts.h"
#include "job_schema_types.h"
#include "jobschema_export.h"

namespace job::schema {

class JOBSCHEMA_EXPORT JobSchemaReader
{
public:
    using Ptr  = std::shared_ptr<JobSchemaReader>;
    using WPtr = std::weak_ptr<JobSchemaReader>;
    using UPtr = std::unique_ptr<JobSchemaReader>;

    JobSchemaReader() = default;
    ~JobSchemaReader() = default;

    JobSchemaReader(const JobSchemaReader &) = delete;
    JobSchemaReader &operator=(const JobSchemaReader &) = delete;
    JobSchemaReader(JobSchemaReader &&) noexcept = default;
    JobSchemaReader &operator=(JobSchemaReader &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobSchemaReader>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobSchemaReader>();
    }

    [[nodiscard]] bool open(io::FactoryType type, const std::string &target);
    void close();

    [[nodiscard]] bool isOpen() const noexcept;

    template <SchemaType T>
    [[nodiscard]] bool read(T &schema, SchemaFormat format)
    {
        std::vector<std::uint8_t> data;

        if (!readAll(data))
            return false;

        return deserialize(schema, data, format);
    }

private:
    [[nodiscard]] bool readAll(std::vector<std::uint8_t> &data);

    template <SchemaType T>
    [[nodiscard]] bool deserialize(T &schema,
                                   const std::vector<std::uint8_t> &data,
                                   SchemaFormat format)
    {
        switch (format) {
        case SchemaFormat::Json: {
            const std::string text(data.begin(), data.end());
            const auto json = nlohmann::json::parse(text);
            return schema.fromJson(json);
        }

        case SchemaFormat::Yaml: {
            const std::string text(data.begin(), data.end());
            return schema.fromYaml(YAML::Load(text));
        }

        case SchemaFormat::Binary: {
            std::span<const std::uint8_t> span{data};
            return schema.fromBinary(span);
        }
        }

        return false;
    }

    io::IODevice::Ptr m_device; // OWNED
};

} // namespace job::schema