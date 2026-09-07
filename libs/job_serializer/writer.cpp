#include "writer.h"

#include <cerrno>

#include <job_logger.h>

namespace job::serializer {

Writer::Writer(const std::filesystem::path &path) :
    m_path(path)
{
}

Writer::~Writer() = default;

bool Writer::writeSchema(const Schema &schema, SerializeFormat mode) noexcept
{
    bool ret = false;

    do {
        if (!schema.isValid()) {
            JOB_LOG_ERROR("[writer] invalid schema, aborting write: {}", pathString());
            break;
        }

        SerializeFormat useMode = mode;
        const auto ext = path().extension().string();

        if (useMode == SerializeFormat::Unknown) {
            if (ext == ".yaml" || ext == ".yml")
                useMode = SerializeFormat::Yaml;
            else if (ext == ".json")
                useMode = SerializeFormat::Json;
            else {
                JOB_LOG_WARN("[writer] unknown extension '{}', defaulting to YAML", ext);
                useMode = SerializeFormat::Yaml;
            }
        }

        switch (useMode) {
        case SerializeFormat::Yaml:
            ret = writeYaml(schema);
            break;
        case SerializeFormat::Json:
            ret = writeJson(schema);
            break;
        case SerializeFormat::Binary:
            ret = writeBinary(schema);
            break;
        case SerializeFormat::Text:
            ret = writeText(schema);
            break;
        default:
            JOB_LOG_ERROR("[writer] invalid writer mode");
            break;
        }

    } while (0);

    return ret;
}

bool Writer::writeEmitter(Emitter &emitter,
                          const Schema &schema,
                          const std::filesystem::path &header_file,
                          const std::filesystem::path &source_file) noexcept
{
    auto [headerContent, sourceContent] = emitter.render(schema);

    setPath(header_file,
            io::JobFile::Access::WriteOnly,
            io::JobFile::OpenMode::Truncate);

    if (!m_file || !m_file->isOpen()) {
        JOB_LOG_ERROR("[writer] failed to open header file: {}", pathString());
        return false;
    }

    if (write(headerContent.data(), headerContent.size()) < 0) {
        JOB_LOG_ERROR("[writer] failed to write header file: {}", pathString());
        closeDevice();
        return false;
    }

    if (!flush()) {
        JOB_LOG_ERROR("[writer] could not flush header file: {}", pathString());
        closeDevice();
        return false;
    }

    closeDevice();

    setPath(source_file,
            io::JobFile::Access::WriteOnly,
            io::JobFile::OpenMode::Truncate);

    if (!m_file || !m_file->isOpen()) {
        JOB_LOG_ERROR("[writer] failed to open source file: {}", pathString());
        return false;
    }

    if (write(sourceContent.data(), sourceContent.size()) < 0) {
        JOB_LOG_ERROR("[writer] failed to write source file: {}", pathString());
        closeDevice();
        return false;
    }

    if (!flush()) {
        JOB_LOG_ERROR("[writer] could not flush source file: {}", pathString());
        closeDevice();
        return false;
    }

    closeDevice();

    return true;
}

bool Writer::writeRuntime(ISerializer &ser,
                          const Schema &schema,
                          const RuntimeObject &object,
                          SerializeFormat fmt) noexcept
{
    if (!schema.isValid())
        return false;

    std::vector<uint8_t> buf;
    if (!ser.encode(schema, object, buf, fmt))
        return false;

    const auto outputPath = path();

    setPath(outputPath,
            io::JobFile::Access::WriteOnly,
            io::JobFile::OpenMode::Truncate);

    if (!m_file || !m_file->isOpen())
        return false;

    if (write(reinterpret_cast<const char *>(buf.data()), buf.size()) < 0) {
        closeDevice();
        return false;
    }

    if (!flush()) {
        closeDevice();
        return false;
    }

    closeDevice();

    return true;
}

std::filesystem::path Writer::path() const
{
    return m_path;
}

std::string Writer::pathString() const
{
    return m_path.string();
}

void Writer::setPath(const std::filesystem::path &path,
                     io::JobFile::Access access,
                     io::JobFile::OpenMode openMode) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_file && m_file->isOpen())
        m_file->closeDevice();

    m_path = path;
    m_file = io::JobFile::createUniq(m_path, access, openMode);

    if (!m_file->openDevice())
        JOB_LOG_ERROR("[writer] failed to open file: {}", pathString());
}

ssize_t Writer::write(const char *data, size_t size)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_file || !m_file->isOpen())
        return -1;

    size_t written = 0;

    while (written < size) {
        const ssize_t result = m_file->write(data + written, size - written);

        if (result < 0) {
            if (errno == EINTR)
                continue;

            return -1;
        }

        if (result == 0)
            return -1;

        written += static_cast<size_t>(result);
    }

    return static_cast<ssize_t>(written);
}

bool Writer::flush()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_file || !m_file->isOpen())
        return true;

    return m_file->flush();
}

void Writer::closeDevice()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_file && m_file->isOpen())
        m_file->closeDevice();
}

bool Writer::writeYaml(const Schema &schema) noexcept
{
    if (!schema.isValid()) {
        JOB_LOG_ERROR("[writer] schema is invalid");
        return false;
    }

    YAML::Emitter emitter;
    Schema::to_yaml(emitter, schema);

    const std::string content = emitter.c_str();

    if (content.empty()) {
        JOB_LOG_ERROR("[writer] empty YAML content for {}", pathString());
        return false;
    }

    const auto outputPath = path();

    setPath(outputPath,
            io::JobFile::Access::WriteOnly,
            io::JobFile::OpenMode::Truncate);

    if (!m_file || !m_file->isOpen()) {
        JOB_LOG_ERROR("[writer] failed to open file for writing: {}", pathString());
        return false;
    }

    if (write(content.data(), content.size()) < 0) {
        JOB_LOG_ERROR("[writer] failed to write YAML schema: {}", pathString());
        closeDevice();
        return false;
    }

    if (!flush()) {
        JOB_LOG_ERROR("[writer] could not flush YAML schema: {}", pathString());
        closeDevice();
        return false;
    }

    closeDevice();

    return true;
}

bool Writer::writeJson(const Schema &schema) noexcept
{
    if (!schema.isValid()) {
        JOB_LOG_ERROR("[writer] schema is invalid");
        return false;
    }

    nlohmann::json j;
    Schema::to_json(j, schema);

    const std::string jsonStr = j.dump(4);
    const auto outputPath = path();

    setPath(outputPath,
            io::JobFile::Access::WriteOnly,
            io::JobFile::OpenMode::Truncate);

    if (!m_file || !m_file->isOpen()) {
        JOB_LOG_ERROR("[writer] failed to open file for writing: {}", pathString());
        return false;
    }

    if (write(jsonStr.data(), jsonStr.size()) < 0) {
        JOB_LOG_ERROR("[writer] failed to write JSON schema: {}", pathString());
        closeDevice();
        return false;
    }

    if (!flush()) {
        JOB_LOG_ERROR("[writer] could not flush JSON schema: {}", pathString());
        closeDevice();
        return false;
    }

    closeDevice();

    return true;
}

bool Writer::writeText([[maybe_unused]] const Schema &schema) noexcept
{
    return false;
}

bool Writer::writeBinary([[maybe_unused]] const Schema &schema) noexcept
{
    return false;
}

} // namespace job::serializer