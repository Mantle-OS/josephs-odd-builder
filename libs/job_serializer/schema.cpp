#include "schema.h"

#include <yaml-cpp/yaml.h>

#include <job_logger.h>

namespace job::serializer {
using json = nlohmann::json;

//////////////////
// JSON
//////////////////
void Schema::to_json(nlohmann::json &j, const Schema &s)
{
    j = json{
        {"tag", s.tag},
        {"version", s.version},
        {"unit", s.unit},
        {"base", s.base},
        {"c_struct", s.c_struct},
        {"include_prefix", s.include_prefix},
        {"out_base", s.out_base},
        {"fields", s.fields}
    };
    if (!s.hdr_name.empty())
        j["hdr_name"] = s.hdr_name.string();
    if (!s.src_name.empty())
        j["src_name"] = s.src_name.string();
    if (!s.schema_path.empty())
        j["schema_path"] = s.schema_path.string();
}
void Schema::from_json(const nlohmann::json &j, Schema &s)
{
    s.tag            = j.value("tag", "");
    s.version        = j.value("version", 0);
    s.unit           = j.value("unit", "");
    s.base           = j.value("base", "");
    s.c_struct       = j.value("c_struct", "");
    s.include_prefix = j.value("include_prefix", "");
    s.out_base       = j.value("out_base", "");
    s.fields         = j.value("fields", std::vector<Field>{});
    // dumb defaults
    s.hdr_name       = j.value("hdr_name", "");
    s.src_name       = j.value("src_name", "");
    s.schema_path    = j.value("schema_path", "");
}

bool Schema::parse(const nlohmann::json &root, Schema &out)
{
    Schema s{};
    try {
        s = root.get<Schema>();
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("[schema] Failed to parse JSON: {}", e.what());
        return false;
    }

    if (s.hdr_name.empty())
        s.hdr_name = s.out_base + ".hpp";

    if (s.src_name.empty())
        s.src_name = s.out_base + ".cpp";

    if (!s.isValid()) {
        JOB_LOG_ERROR("[schema] Parsed JSON schema is not valid");
        s.dump();
        return false;
    }
    out = std::move(s);
    return true;
}


//////////////////
// YAML
//////////////////
void Schema::to_yaml(YAML::Emitter &emitter, const Schema &s) noexcept
{
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "tag"              << YAML::Value << s.tag;
    emitter << YAML::Key << "version"          << YAML::Value << s.version;
    emitter << YAML::Key << "unit"             << YAML::Value << s.unit;
    emitter << YAML::Key << "base"             << YAML::Value << s.base;
    emitter << YAML::Key << "c_struct"         << YAML::Value << s.c_struct;
    emitter << YAML::Key << "include_prefix"   << YAML::Value << s.include_prefix;
    emitter << YAML::Key << "out_base"         << YAML::Value << s.out_base;
    // Filenames are optional, so we'll only emit them if they have content
    if (!s.hdr_name.empty())
        emitter << YAML::Key << "hdr_name" << YAML::Value << s.hdr_name.string();
    if (!s.src_name.empty())
        emitter << YAML::Key << "src_name" << YAML::Value << s.src_name.string();

    emitter << YAML::Key << "fields";
    emitter << YAML::BeginSeq;
    for (const auto &f : s.fields)
        Field::to_yaml(emitter, f);

    emitter << YAML::EndSeq;

    emitter << YAML::EndMap;
}

void Schema::dump() noexcept
{
    JOB_LOG_INFO("----- [Schema Dump] -----");
    JOB_LOG_INFO("tag: {}", tag);
    JOB_LOG_INFO("version: {}", version);
    JOB_LOG_INFO("unit: {}", unit);
    JOB_LOG_INFO("base: {}", base);
    JOB_LOG_INFO("c_struct: {}", c_struct);
    JOB_LOG_INFO("include_prefix: {}", include_prefix);
    JOB_LOG_INFO("out_base: {}", out_base);
    JOB_LOG_INFO("hdr_name: {}", hdr_name.string());
    JOB_LOG_INFO("src_name: {}", src_name.string());
    JOB_LOG_INFO("fields: {}", fields.size());

    for (const auto &f : fields)
        f.dump();

    JOB_LOG_INFO("-------------------------");
}

//////////////////
// UTILS
//////////////////
bool Schema::parse(const YAML::Node &root, Schema &out) noexcept
{
    bool ret = false;
    Schema s{};

    do {
        if (!root || !root.IsMap()) {
            JOB_LOG_ERROR("[schema] YAML root node is not a map");
            break;
        }

        auto getString = [&](const char* key) {
            if (!root[key]) {
                JOB_LOG_WARN("[schema] YAML missing required key: {}", key);
                return std::string{};
            }
            return root[key].as<std::string>("");
        };

        s.tag = getString("tag");
        s.unit = getString("unit");
        s.base = getString("base");
        s.c_struct = getString("c_struct");
        s.include_prefix = getString("include_prefix");
        s.out_base = getString("out_base");

        if (!root["version"])
            break;

        s.version = root["version"].as<int>(0);

        if (!Field::from_yaml(root["fields"], s.fields)) {
            JOB_LOG_ERROR("[schema] Failed to parse 'fields' from YAML");
            break;
        }

        // Handle optional file paths
        s.hdr_name = root["hdr_name"] ? root["hdr_name"].as<std::string>("") : (s.out_base + ".hpp");
        s.src_name = root["src_name"] ? root["src_name"].as<std::string>("") : (s.out_base + ".cpp");
        s.schema_path = root["schema_path"] ? root["schema_path"].as<std::string>("") : "";

        if (!s.isValid()) {
            JOB_LOG_ERROR("[schema] Parsed YAML schema is not valid");
            s.dump();
            break;
        }

        ret = true;
        out = std::move(s);

    } while (0);

    return ret;
}

bool Schema::isValid() const noexcept
{
    bool ret = false;

    do {
        if (tag.empty())
            break;

        if (version <= 0)
            break;

        if (unit.empty())
            break;

        if (base.empty())
            break;

        if (c_struct.empty())
            break;

        if (include_prefix.empty())
            break;

        if (out_base.empty())
            break;

        if (fields.empty()) {
            JOB_LOG_WARN("[schema] Validation failed: Schema contains no fields.");
            break;
        }

        if (!Field::isValid(fields))
            break;

        ret = true;
    } while (0);

    return ret;
}

} // namespace job::serializer
