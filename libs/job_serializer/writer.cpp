#include "writer.h"
#include "iserializer.h"

#include <job_logger.h>

namespace job::serializer {

Writer::Writer(const std::filesystem::path &path) :
    io::FileIO{path, io::FileMode::RegularFile, true}
{
}

bool Writer::writeSchema(const Schema &schema, SerializeFormat mode) noexcept
{
    bool ret = false;

    do {
        if (!schema.isValid()) {
            JOB_LOG_ERROR("[writer] invalid schema, aborting write: {}", pathString());
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
                JOB_LOG_WARN("[writer] unknown extension '{}', defaulting to YAML", ext.c_str());
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

    if(isOpen())
        closeDevice();

    setPath(header_file, io::OpenType::Truncate);
    if (isOpen()) {
        if (write(headerContent.data(), headerContent.size()) < 0) {
            JOB_LOG_ERROR("[writer] failed to write header file: {}", pathString());
            return false;
        }else{
            closeDevice();
            if(!flush()){
                JOB_LOG_ERROR("[writer] could not flush the {}", pathString());
                return false;
            }
        }
    }else{
        return false;
    }

    setPath(source_file, io::OpenType::Truncate);
    if(isOpen()){
        if (write(sourceContent.data(), sourceContent.size()) < 0) {
            JOB_LOG_ERROR("[writer] failed to write source file : {}", pathString());
            return false;
        }else{
            closeDevice();
            if(!flush()){
                JOB_LOG_ERROR("[writer] could not flush the {}", pathString());
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


    setPath(path(), io::OpenType::Truncate);
    if(isOpen()){
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

bool Writer::writeYaml(const Schema &schema) noexcept
{
    if(!schema.isValid()){
        JOB_LOG_ERROR("[writer] schema is invalid {}");
        return false;
    }
    YAML::Emitter emitter;
    Schema::to_yaml(emitter, schema);
    const std::string content = emitter.c_str();
    if (content.empty()) {
        JOB_LOG_ERROR("[writer] empty YAML content for {}", pathString());
        return false;
    }

    setPath(path(), io::OpenType::Truncate);
    if (isOpen()) {
        if (write(content.data(), content.size()) < 0) {
            JOB_LOG_ERROR("[writer] failed to write YAML schema: {}", pathString());
            return false;
        }

        closeDevice();
        if(!flush()){
            JOB_LOG_INFO("[writer] Could not write to disk YAML schema: {}", pathString());
            return false;
        }
    } else {
        JOB_LOG_ERROR("[writer] failed to open file for writing: {}", pathString());
        return false;
    }

    return true;
}

bool Writer::writeJson(const Schema &schema) noexcept
{
    if(!schema.isValid()){
        JOB_LOG_ERROR("[writer] schema is invalid {}");
        return false;
    }
    nlohmann::json j;
    Schema::to_json(j, schema);
    setPath(path(), io::OpenType::Truncate);
    if (isOpen()) {
        std::string jsonStr = j.dump(4);
        if (write(jsonStr.data(), jsonStr.size()) < 0) {
            JOB_LOG_ERROR("[writer] failed to write JSON schema: {}", pathString());
            return false;
        }
        closeDevice();
        if(!flush()){
            JOB_LOG_ERROR("[writer] could not flush data to disk {}", pathString());
            return false;
        }
    } else {
        JOB_LOG_ERROR("[writer] failed to open file for writing: {}", pathString());
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

} // namespace job::serializer
// CHECKPOINT: v1
