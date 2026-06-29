#include "writer.h"
#include "iserializer.h"
#include "job_serializer_logger.h"

namespace job::serializer {

Writer::Writer(const std::filesystem::path &path) :
   m_path{path}
{
}

bool Writer::writeSchema(const Schema &schema, SerializeFormat mode) noexcept
{
    bool ret = false;

    do {
        if (!schema.isValid()) {
            JOB_SER_ERROR("[writer] invalid schema, aborting write: {}", pathString());
            break;
        }

        // Determine the file extension and set mode accordingly
        SerializeFormat useMode = mode;
        const auto ext = path().extension().string();

        if (useMode == SerializeFormat::Unknown) {
            if (ext == ".yaml" || ext == ".yml")
                useMode = SerializeFormat::Yaml;
            else if (ext == ".json")
                useMode = SerializeFormat::Json;
            else {
                JOB_SER_WARN("[writer] unknown extension '{}', defaulting to YAML", ext.c_str());
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
            JOB_SER_ERROR("[writer] invalid writer mode");
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

    if(m_open)
        closeDevice();

    setPath(header_file, WriteType::Truncate);
    if (m_open) {
        if (write(headerContent.data(), headerContent.size()) < 0) {
            JOB_SER_ERROR("[writer] failed to write header file: {}", pathString());
            return false;
        }else{
            closeDevice();
            if(!flush()){
                JOB_SER_ERROR("[writer] could not flush the {}", pathString());
                return false;
            }
        }
    }else{
        return false;
    }

    setPath(source_file, WriteType::Truncate);
    if(m_open){
        if (write(sourceContent.data(), sourceContent.size()) < 0) {
            JOB_SER_ERROR("[writer] failed to write source file : {}", pathString());
            return false;
        }else{
            closeDevice();
            if(!flush()){
                JOB_SER_ERROR("[writer] could not flush the {}", pathString());
                return false;
            }
        }
    }else{
        return false;
    }

    return true;
}

bool Writer::writeRuntime(ISerializer &ser,
                          const Schema &schema,
                          const RuntimeObject &object,
                          SerializeFormat fmt) noexcept
{
    if(!schema.isValid())
        return false;

    std::vector<uint8_t> buf;
    if (!ser.encode(schema, object, buf, fmt))
        return false;


    setPath(path(), WriteType::Truncate);
    if(m_open){
        if (write(reinterpret_cast<const char*>(buf.data()), buf.size()) < 0)
            return false;

        closeDevice();
        if(!flush())
            return false;

    }else{
        return false;
    }
    return true;
}

std::filesystem::path Writer::path()
{
    return m_path;
}

std::string Writer::pathString() const
{
    return m_path.string();
}

ssize_t Writer::write(const char *data, size_t size)
{
    if (!m_open)
        return -1;


    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_output.write(data, size)) {
        return size;
    } else {
        return -1;
    }
}

bool Writer::flush()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    bool ret = true;

    if (!m_open)
        return true;

    if (m_writeMode && m_output.is_open()) {
        m_output.flush();
        ret = !m_output.fail();
    }
    return ret;
}

void Writer::setPath(const std::filesystem::path &path, WriteType openType) noexcept
{
    if (m_open)
        closeDevice();

    m_path = path;
    m_writeMode = (openType != WriteType::ReadOnly);

    if (openType != WriteType::ReadOnly && !std::filesystem::exists(path)) {
        std::ofstream(path.string()); // Create
        JOB_SER_INFO("[Serlizer Writer] Created file: {}", path.string());
    }

    switch (openType) {
    case WriteType::Truncate:
        m_output.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
        m_open = m_output.is_open();
        break;
    case WriteType::Append:
        m_output.open(path, std::ios::out | std::ios::binary | std::ios::app);
        m_open = m_output.is_open();
        break;
    case WriteType::ReadOnly:
        m_input.open(path, std::ios::in | std::ios::binary);
        m_open = m_input.is_open();
        break;
    }

    if (!m_open) {
        JOB_SER_ERROR("[FileIO] Failed to open file: {}", pathString());
    }
}

bool Writer::writeYaml(const Schema &schema) noexcept
{
    if(!schema.isValid()){
        JOB_SER_ERROR("[writer] schema is invalid {}");
        return false;
    }
    YAML::Emitter emitter;
    Schema::to_yaml(emitter, schema);
    const std::string content = emitter.c_str();
    if (content.empty()) {
        JOB_SER_ERROR("[writer] empty YAML content for {}", pathString());
        return false;
    }

    setPath(path(), WriteType::Truncate);
    if (m_open) {
        if (write(content.data(), content.size()) < 0) {
            JOB_SER_ERROR("[writer] failed to write YAML schema: {}", pathString());
            return false;
        }

        closeDevice();
        if(!flush()){
            JOB_SER_INFO("[writer] Could not write to disk YAML schema: {}", pathString());
            return false;
        }
    } else {
        JOB_SER_ERROR("[writer] failed to open file for writing: {}", pathString());
        return false;
    }

    return true;
}

bool Writer::writeJson(const Schema &schema) noexcept
{
    if(!schema.isValid()){
        JOB_SER_ERROR("[writer] schema is invalid {}");
        return false;
    }
    nlohmann::json j;
    Schema::to_json(j, schema);
    setPath(m_path, WriteType::Truncate);

    if (m_open) {
        std::string jsonStr = j.dump(4);
        if (write(jsonStr.data(), jsonStr.size()) < 0) {
            JOB_SER_ERROR("[writer] failed to write JSON schema: {}", pathString());
            return false;
        }
        closeDevice();
        if(!flush()){
            JOB_SER_ERROR("[writer] could not flush data to disk {}", pathString());
            return false;
        }
    } else {
        JOB_SER_ERROR("[writer] failed to open file for writing: {}", pathString());
        return false;
    }

    return true;
}

// STUB FIXES for Text and Binary (still need to refactor using FileIO)
bool Writer::writeText([[maybe_unused]] const Schema &schema) noexcept
{
    bool ret = false;
    return ret;
}

bool Writer::writeBinary([[maybe_unused]] const Schema &schema) noexcept
{
    bool ret = false;
    return ret;
}

void Writer::closeDevice()
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

} // namespace job::serializer

