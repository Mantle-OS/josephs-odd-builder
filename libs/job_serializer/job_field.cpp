#include "job_field.h"

namespace job::serializer{

std::optional<int> Field::parseBinSize(const std::string &type) noexcept
{
    std::optional<int> ret;

    do {
        if (!type.starts_with("bin[") || !type.ends_with("]"))
            break;

        const auto inner = type.substr(4, type.size() - 5);

        try {
            int v = std::stoi(inner);
            if (v > 0)
                ret = v;
        } catch (...) {
            JOB_LOG_WARN("[schema] invalid bin size: {}", type);
        }

    } while (0);

    return ret;
}

std::optional<int> Field::parseListBinSize(const std::string &type) noexcept
{
    std::optional<int> ret;

    do {
        if (!type.starts_with("list<bin[") || !type.ends_with("]>"))
            break;

        const auto inner = type.substr(9, type.size() - 11);

        try {
            int v = std::stoi(inner);
            if (v > 0)
                ret = v;
        } catch (...) {
            JOB_LOG_WARN("[schema] invalid list<bin[N]> size: {}", type);
        }

    } while (0);

    return ret;
}

bool Field::from_yaml(const YAML::Node &n, Field &out) noexcept
{
    bool ret = false;

    do {
        if (!n || !n.IsMap()) {
            JOB_LOG_WARN("[schema] skipping non-map field entry");
            break;
        }

        if (!n["name"] || !n["type"]) {
            JOB_LOG_ERROR("[schema] field missing 'name' or 'type'");
            break;
        }
        if (n["key"])
            out.key = n["key"].as<uint32_t>(0);

        out.name = n["name"].as<std::string>("");
        out.type = n["type"].as<std::string>("");
        out.kind = deduceFieldKind(out.type);

        if (n["required"])
            out.required = n["required"].as<bool>(false);

        if (n["comment"])
            out.comment = n["comment"].as<std::string>();

        if (n["size"])
            out.size = n["size"].as<int>();

        if (n["ctype"])
            out.ctype = n["ctype"].as<std::string>();

        if (n["ref_include"])
            out.ref_include = n["ref_include"].as<std::string>();

        if (n["ref_sym"])
            out.ref_sym = n["ref_sym"].as<std::string>();

        // if ((out.kind == FieldKind::Struct || out.kind == FieldKind::ListStruct) && out.ref_include && !out.ref_sym){
        //     JOB_LOG_ERROR("[schema] Field '{}' is a struct with 'ref_include' but is missing 'ref_sym'", out.name);
        //     break; // Fails the parse
        // }


        if (!out.ref_sym && out.ref_include) {
            auto base = std::filesystem::path(*out.ref_include).filename().string();

            if (base.ends_with(".hpp"))
                base = base.substr(0, base.size() - 4);

            if (base.starts_with("mpkg_mp_"))
                base = base.substr(8);

            out.ref_sym = base;
        }

        if (out.kind == FieldKind::Bin && !out.size)
            out.size = parseBinSize(out.type);
        else if (out.kind == FieldKind::ListBin && !out.size)
            out.size = parseListBinSize(out.type);

        ret = true;

    } while (0);

    return ret;
}

bool Field::from_yaml(const YAML::Node &node, std::vector<Field> &fields) noexcept
{
    bool ret = false;

    do {
        if (!node) {
            JOB_LOG_ERROR("[schema] 'fields' node missing");
            break;
        }

        if (node.IsSequence()) {
            for (const auto &n : node) {
                Field f{};
                if (!Field::from_yaml(n, f)) {
                    JOB_LOG_WARN("[schema] skipping invalid field entry");
                    continue;
                }
                fields.push_back(std::move(f));
            }
        } else if (node.IsMap()) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                const std::string keyStr = it->first.as<std::string>("");
                const YAML::Node &value = it->second;

                Field f{};

                try {
                    f.key = static_cast<uint32_t>(std::stoul(keyStr));
                } catch (...) {
                    JOB_LOG_ERROR("[schema] invalid field key: {}", keyStr);
                    break;
                }

                if (!Field::from_yaml(value, f))
                    break;

                fields.push_back(std::move(f));
            }

        } else {
            JOB_LOG_ERROR("[schema] 'fields' must be a map or sequence");
            break;
        }

        ret = !fields.empty();

    } while (0);

    return ret;
}

void Field::to_yaml(YAML::Emitter &emitter, const Field &f) noexcept
{
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "key"       << YAML::Value << f.key;
    emitter << YAML::Key << "name"      << YAML::Value << f.name;
    emitter << YAML::Key << "type"      << YAML::Value << f.type;
    // 'kind' is deduced from 'type', so we don't emit it
    emitter << YAML::Key << "required"  << YAML::Value << f.required;

    if (f.size)
        emitter << YAML::Key << "size" << YAML::Value << *f.size;
    if (f.ctype)
        emitter << YAML::Key << "ctype" << YAML::Value << *f.ctype;
    if (f.ref_include)
        emitter << YAML::Key << "ref_include" << YAML::Value << *f.ref_include;
    if (f.ref_sym)
        emitter << YAML::Key << "ref_sym" << YAML::Value << *f.ref_sym;
    if (f.comment)
        emitter << YAML::Key << "comment" << YAML::Value << *f.comment;

    emitter << YAML::EndMap;
}


/// JSON
void Field::to_json(nlohmann::json &j, const Field &f)
{
    j = nlohmann::json{
        {"key", f.key},
        {"name", f.name},
        {"type", f.type},
        // {"kind", static_cast<uint8_t>(f.kind)},
        {"required", f.required}
    };

    if (f.size)
        j["size"] = *f.size;
    if (f.ctype)
        j["ctype"] = *f.ctype;
    if (f.ref_include)
        j["ref_include"] = *f.ref_include;
    if (f.ref_sym)
        j["ref_sym"] = *f.ref_sym;
    if (f.comment)
        j["comment"] = *f.comment;
}

void Field::from_json(const nlohmann::json &j, Field &f)
{
    f.key       = j.value("key", 0);
    f.name      = j.value("name", "");
    f.type      = j.value("type", "");
    f.kind = deduceFieldKind(f.type);
    f.required  = j.value("required", false);

    if (j.contains("size"))
        f.size = j["size"].get<int>();
    if (j.contains("ctype"))
        f.ctype = j["ctype"].get<std::string>();
    if (j.contains("ref_include"))
        f.ref_include = j["ref_include"].get<std::string>();
    if (j.contains("ref_sym"))
        f.ref_sym = j["ref_sym"].get<std::string>();
    if (j.contains("comment"))
        f.comment = j["comment"].get<std::string>();

    // Auto-deduce ref_sym
    if (!f.ref_sym && f.ref_include) {
        auto base = std::filesystem::path(*f.ref_include).filename().string();
        if (base.ends_with(".hpp"))
            base = base.substr(0, base.size() - 4);
        if (base.starts_with("mpkg_"))
            base = base.substr(5);
        f.ref_sym = base;
    }
    // Auto-deduce size
    if (f.kind == FieldKind::Bin && !f.size)
        f.size = parseBinSize(f.type);
    else if (f.kind == FieldKind::ListBin && !f.size)
        f.size = parseListBinSize(f.type);


}

void Field::dump() const noexcept
{
    JOB_LOG_DEBUG("  Field #{}", key);
    JOB_LOG_DEBUG("    name: {}", name);
    JOB_LOG_DEBUG("    type: {}", type);
    JOB_LOG_DEBUG("    kind: {}", static_cast<int>(kind));

    if (size)
        JOB_LOG_DEBUG("    size: {}", *size);

    if (ctype)
        JOB_LOG_DEBUG("    ctype: {}", *ctype);

    if (ref_include)
        JOB_LOG_DEBUG("    ref_include: {}", *ref_include);

    if (ref_sym)
        JOB_LOG_DEBUG("    ref_sym: {}", *ref_sym);

    JOB_LOG_DEBUG("    required: {}", required ? "true" : "false");

    if (comment)
        JOB_LOG_DEBUG("    comment: {}", *comment);
}

bool Field::isValid(std::vector<Field> fields) noexcept
{
    for (const auto &f : fields) {
        if (f.name.empty() || f.type.empty()) {
            JOB_LOG_WARN("[schema] Validation failed: Field key {} is missing 'name' or 'type'", f.key);
            return false;
        }

        if (f.kind == FieldKind::Struct || f.kind == FieldKind::ListStruct) {
            if (!f.ref_sym) {
                JOB_LOG_ERROR("[schema] Validation failed: Field '{}' is a struct but is missing 'ref_sym'", f.name);
                return false;
            }
        }
    }
    return true;
}
} // namespoace job::serializer
// CHECKPOINT: v1
