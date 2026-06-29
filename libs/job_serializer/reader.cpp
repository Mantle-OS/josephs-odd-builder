#include "reader.h"
#include "job_serializer_logger.h"

namespace job::serializer {

Reader::Reader(const std::filesystem::path &path) :
    m_path(path)
{
}

Reader::~Reader()
{
    if(m_open)
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
            JOB_SER_WARN("[reader] unknown extension '{}', defaulting to YAML", ext);
            useMode = SerializeFormat::Yaml;
        }
    }
    switch (useMode) {
    case SerializeFormat::Yaml:
        return readYaml(out_schema);
    case SerializeFormat::Json:
        return readJson(out_schema);
    default:
        JOB_SER_ERROR("[reader] invalid reader mode for schema: {}", (int)useMode);
        return false;
    }
}

bool Reader::readEmitter(const Emitter &in_emitter,
                         Schema &out_schema) noexcept
{
    Schema temp_schema = in_emitter.lastSchema();
    if (!temp_schema.isValid()) {
        JOB_SER_ERROR("[reader] Emitter did not have a valid schema cached.");
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
        JOB_SER_ERROR("[reader] Failed to read runtime file: {}", pathString());
        return false;
    }

    if (buf.empty()) {
        JOB_SER_WARN("[reader] File was empty: {}", pathString());
        return false;
    }
    return ser.decode(schema, object, buf, fmt);
}

bool Reader::readYaml(Schema &out_schema) noexcept
{
    const std::string content = readText(out_schema);
    if (content.empty()) {
        JOB_SER_WARN("[reader] File was empty or unreadable: {}", pathString());
        return false;
    }

    try {
        YAML::Node node = YAML::Load(content);
        return Schema::parse(node, out_schema);
    } catch (const std::exception &e) {
        JOB_SER_ERROR("[reader] YAML parse error: {}", e.what());
        return false;
    }
}

bool Reader::readJson(Schema &out_schema) noexcept
{
    const std::string content = readText(out_schema);
    if (content.empty()) {
        JOB_SER_WARN("[reader] File was empty or unreadable: {}", pathString());
        return false;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(content);
        return Schema::parse(j, out_schema);
    } catch (const std::exception &e) {
        JOB_SER_ERROR("[reader] JSON parse error: {}", e.what());
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

std::string Reader::readAll() noexcept
{
    if (m_open)
        closeDevice();

    m_input.open(m_path, std::ios::in | std::ios::binary);
    m_open = m_input.is_open();

    std::ostringstream ss;
    char buffer[4096];
    ssize_t bytesRead = 0;

    m_input.clear();
    m_input.seekg(0);

    while ((bytesRead = read(buffer, sizeof(buffer))) > 0)
        ss.write(buffer, bytesRead);

    if (bytesRead < 0) {
        JOB_SER_ERROR("[FileIO] Error during readAll: {}", pathString());
        return "";
    }
    return ss.str();
}

bool Reader::readAll(std::vector<uint8_t> &out_buf) noexcept
{

    if (m_open)
        closeDevice();

    m_input.open(m_path, std::ios::in | std::ios::binary);
    m_open = m_input.is_open();

    std::string content = readAll();
    if (content.empty() && !m_open)  // okay for thgreads ?
        return false;

    out_buf.assign(content.begin(), content.end());
    return true;
}

// main read
ssize_t Reader::read(char *buffer, size_t size)
{
    if (!m_open)
        return -1;

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_input.read(buffer, size))
        return m_input.gcount();
    else
        return m_input.gcount();

}

void Reader::closeDevice()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_open)
        return;

    if (m_output.is_open()) {
        m_output.flush();
        m_output.close();
    }
    if (m_input.is_open())
        m_input.close();

    m_open = false;

    // unlock ?
}

std::filesystem::path Reader::path() const
{
    return m_path;
}

} // namespace job::serializer
