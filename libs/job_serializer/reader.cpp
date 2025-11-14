#include "reader.h"
#include <job_logger.h>

namespace job::serializer {

Reader::Reader(const std::filesystem::path &path) :
    io::FileIO{path, io::FileMode::RegularFile, false}
{
}

Reader::~Reader()
{
    if(isOpen())
        closeDevice();
}

bool Reader::readSchema(Schema &out_schema, SerializeFormat mode) noexcept
{
    SerializeFormat useMode = mode;
    const auto ext = path().extension().string();

    if (useMode == SerializeFormat::Unknown) {
        if (ext == ".yaml" || ext == ".yml")
            useMode = SerializeFormat::Yaml;
        else if (ext == ".json")
            useMode = SerializeFormat::Json;
        else {
            JOB_LOG_WARN("[reader] unknown extension '{}', defaulting to YAML", ext);
            useMode = SerializeFormat::Yaml;
        }
    }
    switch (useMode) {
    case SerializeFormat::Yaml:
        return readYaml(out_schema);
    case SerializeFormat::Json:
        return readJson(out_schema);
    default:
        JOB_LOG_ERROR("[reader] invalid reader mode for schema: {}", (int)useMode);
        return false;
    }
}

bool Reader::readEmitter(const Emitter &in_emitter,
                         Schema &out_schema) noexcept
{
    Schema temp_schema = in_emitter.lastSchema();
    if (!temp_schema.isValid()) {
        JOB_LOG_ERROR("[reader] Emitter did not have a valid schema cached.");
        return false;
    }

    out_schema = std::move(temp_schema);
    return true;
}

bool Reader::readRuntime(ISerializer &ser, const Schema &schema,
                         RuntimeObject &object, SerializeFormat fmt) noexcept
{
    std::vector<uint8_t> buf;
    if (!readAll(buf)) {
        JOB_LOG_ERROR("[reader] Failed to read runtime file: {}", pathString());
        return false;
    }

    if (buf.empty()) {
        JOB_LOG_WARN("[reader] File was empty: {}", pathString());
        return false;
    }
    return ser.decode(schema, object, buf, fmt);
}

bool Reader::readYaml(Schema &out_schema) noexcept
{
    const std::string content = readText(out_schema);
    if (content.empty()) {
        JOB_LOG_WARN("[reader] File was empty or unreadable: {}", pathString());
        return false;
    }

    try {
        YAML::Node node = YAML::Load(content);
        return Schema::parse(node, out_schema);
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("[reader] YAML parse error: {}", e.what());
        return false;
    }
}

bool Reader::readJson(Schema &out_schema) noexcept
{
    const std::string content = readText(out_schema);
    if (content.empty()) {
        JOB_LOG_WARN("[reader] File was empty or unreadable: {}", pathString());
        return false;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(content);
        return Schema::parse(j, out_schema);
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("[reader] JSON parse error: {}", e.what());
        return false;
    }
}

std::string Reader::readText(Schema &in_schema)
{
    m_lastRead = in_schema;
    return readAll();
}

} // namespace job::serializer
// CHECKPOINT: v2
