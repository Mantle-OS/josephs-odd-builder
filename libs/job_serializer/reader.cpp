#include "reader.h"

#include <job_logger.h>

namespace job::serializer {

Reader::Reader(const std::filesystem::path &path) :
    m_path(path),
    m_file(io::JobFile::createUniq(
        path,
        io::JobFile::Access::ReadOnly,
        io::JobFile::OpenMode::OpenExisting))
{
}

Reader::~Reader() = default;

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
        JOB_LOG_ERROR("[reader] invalid reader mode for schema: {}", static_cast<int>(useMode));
        return false;
    }
}

bool Reader::readEmitter(const Emitter &in_emitter, Schema &out_schema) noexcept
{
    Schema temp_schema = in_emitter.lastSchema();

    if (!temp_schema.isValid()) {
        JOB_LOG_ERROR("[reader] Emitter did not have a valid schema cached.");
        return false;
    }

    out_schema = std::move(temp_schema);
    return true;
}

bool Reader::readRuntime(ISerializer &ser,
                         const Schema &schema,
                         RuntimeObject &object,
                         SerializeFormat fmt) noexcept
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

std::string Reader::pathString() const
{
    return m_path.string();
}

std::filesystem::path Reader::path() const
{
    return m_path;
}

std::string Reader::readAll() noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string output;

    if (!m_file)
        return output;

    if (m_file->isOpen())
        m_file->closeDevice();

    if (!m_file->openDevice()) {
        JOB_LOG_ERROR("[reader] Failed to open file: {}", pathString());
        return output;
    }

    if (m_file->readAll(output) < 0) {
        JOB_LOG_ERROR("[reader] Error during readAll: {}", pathString());
        output.clear();
    }

    return output;
}

bool Reader::readAll(std::vector<uint8_t> &out_buf) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);

    out_buf.clear();

    if (!m_file)
        return false;

    if (m_file->isOpen())
        m_file->closeDevice();

    if (!m_file->openDevice()) {
        JOB_LOG_ERROR("[reader] Failed to open file: {}", pathString());
        return false;
    }

    if (m_file->readAll(out_buf) < 0) {
        JOB_LOG_ERROR("[reader] Error during readAll: {}", pathString());
        out_buf.clear();
        return false;
    }

    return true;
}

ssize_t Reader::read(char *buffer, size_t size)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_file || !m_file->isOpen())
        return -1;

    return m_file->read(buffer, size);
}

} // namespace job::serializer