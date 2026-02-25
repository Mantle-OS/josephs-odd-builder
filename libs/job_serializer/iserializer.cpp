#include "iserializer.h"

#include <nlohmann/json.hpp>

#include <job_logger.h>

#include "schema.h"
#include "job_serializer_utils.h"

namespace job::serializer {

using json = nlohmann::json;
//////////////////////////////
// JSON Helpers
/////////////////////////////

[[nodiscard]] static bool fieldValueToJson( const FieldValue& fv, nlohmann::json &out_j) noexcept
{
    try {
        if (fv.isScalar()) {
            out_j = jsonToScalar(std::get<FieldValue::Scalar>(fv.value));
        } else if (fv.isBinary()) {
            out_j = nlohmann::json::binary(std::get<FieldValue::Binary>(fv.value));
        } else if (fv.isList()) {
            out_j = nlohmann::json::array();
            for (const auto& item : std::get<FieldValue::List>(fv.value)) {
                nlohmann::json j_item;
                if (!fieldValueToJson(item, j_item))  {
                    JOB_LOG_WARN("[serializer] Failed to serialize item in list");
                    continue;
                }
                out_j.push_back(j_item);
            }
        }
        else if (fv.isStruct()) {
            out_j = nlohmann::json::object();
            for (const auto& [key, val] : std::get<FieldValue::Struct>(fv.value)) {
                nlohmann::json j_val;
                if (!fieldValueToJson(val, j_val))  {
                    JOB_LOG_WARN("[serializer] Failed to serialize value for key '{}'", key);
                    continue;
                }
                out_j[key] = j_val;
            }
        } else {
            out_j = nullptr;
        }
        return true;
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("[serializer] FieldValue to JSON conversion error: {}", e.what());
        return false;
    }
}

static bool jsonToFieldValue_DumbRecursive(const json& j_val, FieldValue& out_fv)
{
    try {
        if (j_val.is_null()) {
            out_fv.value = std::monostate{};
        } else if (j_val.is_string()) {
            out_fv.value = FieldValue::Scalar{ j_val.get<std::string>() };
        } else if (j_val.is_boolean()) {
            out_fv.value = FieldValue::Scalar{ (uint32_t)j_val.get<bool>() };
        } else if (j_val.is_number_integer()) {
            out_fv.value = FieldValue::Scalar{ j_val.get<int64_t>() };
        } else if (j_val.is_number_unsigned()) {
            out_fv.value = FieldValue::Scalar{ j_val.get<uint64_t>() };
        } else if (j_val.is_binary()) {
            out_fv.value = FieldValue::Binary(j_val.get_binary().begin(), j_val.get_binary().end());
        } else if (j_val.is_array()) {
            FieldValue::List list;
            for (const auto& j_item : j_val) {
                FieldValue list_fv;
                if (jsonToFieldValue_DumbRecursive(j_item, list_fv)) {
                    list.push_back(list_fv);
                }
            }
            out_fv.value = list;
        } else if (j_val.is_object()) {
            FieldValue::Struct map;
            for (auto it = j_val.begin(); it != j_val.end(); ++it) {
                FieldValue struct_fv;
                if (jsonToFieldValue_DumbRecursive(it.value(), struct_fv))
                    map[it.key()] = struct_fv;
            }
            out_fv.value = map;
        } else {
            return false; // Unknown type
        }
        return true;
    } catch(const std::exception& e) {
        JOB_LOG_ERROR("[serializer] Recursive JSON parse error: {}", e.what());
        return false;
    }
}

/////////////////////////////
// YAML Helpers
/////////////////////////////
// Helper for scalar-to-YAML conversion
[[nodiscard]] static bool scalarToYaml( const FieldValue::Scalar &scalar,  YAML::Emitter &out_emitter) noexcept
{
    auto visitor = [&](const auto &value){
        out_emitter << value;
    };
    std::visit(visitor, scalar);
    return true;
}

// Recursive helper for FieldValue -> YAML::Emitter
[[nodiscard]] static bool fieldValueToYaml( const FieldValue& fv,  YAML::Emitter& out_emitter) noexcept
{
    try {
        if (fv.isScalar()) {
            if(!scalarToYaml(std::get<FieldValue::Scalar>(fv.value), out_emitter))
                return false;
        } else if (fv.isBinary()) {
            // YAML doesn't have a great binary type, emit as hex string
            const auto& bin = std::get<FieldValue::Binary>(fv.value);
            std::stringstream ss;
            ss << std::hex << std::setfill('0');
            for(uint8_t byte : bin)
                ss << std::setw(2) << static_cast<int>(byte);
            out_emitter << ss.str();
        } else if (fv.isList()) {
            out_emitter << YAML::BeginSeq;
            for (const auto& item : std::get<FieldValue::List>(fv.value)) {
                if (!fieldValueToYaml(item, out_emitter))
                    JOB_LOG_WARN("[serializer] Failed to serialize item in YAML list");
            }
            out_emitter << YAML::EndSeq;
        } else if (fv.isStruct()) {
            out_emitter << YAML::BeginMap;
            for (const auto& [key, val] : std::get<FieldValue::Struct>(fv.value)) {
                out_emitter << YAML::Key << key;
                out_emitter << YAML::Value;
                if (!fieldValueToYaml(val, out_emitter))
                    JOB_LOG_WARN("[serializer] Failed to serialize value for key '{}' in YAML map", key);
            }
            out_emitter << YAML::EndMap;
        } else {
            out_emitter << YAML::Null;
        }
        return true;
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("[serializer] FieldValue to YAML conversion error: {}", e.what());
        return false;
    }
}

static bool yamlToFieldValue_DumbRecursive(const YAML::Node& y_val, FieldValue& out_fv)
{
    try  {
        if (y_val.IsNull()) {
            out_fv.value = std::monostate{};
        } else if (y_val.IsScalar()) {
            std::string s = y_val.as<std::string>();
            // This is a weak spot, but for a "dumb" parser, it's what we have
            out_fv.value = FieldValue::Scalar{s};
            // We could try to parse numbers, but string is safest
        } else if (y_val.IsSequence()) {
            FieldValue::List list;
            for (const auto& y_item : y_val) {
                FieldValue list_fv;
                if (yamlToFieldValue_DumbRecursive(y_item, list_fv)) {
                    list.push_back(list_fv);
                }
            }
            out_fv.value = list;
        } else if (y_val.IsMap()) {
            FieldValue::Struct map;
            for (auto it = y_val.begin(); it != y_val.end(); ++it) {
                std::string key = it->first.as<std::string>();
                FieldValue struct_fv;
                if (yamlToFieldValue_DumbRecursive(it->second, struct_fv)) {
                    map[key] = struct_fv;
                }
            }
            out_fv.value = map;
        } else {
            return false; // Unknown type
        }
        return true;
    }  catch(const std::exception& e) {
        JOB_LOG_ERROR("[serializer] Recursive YAML parse error: {}", e.what());
        return false;
    }
}

/////////////////////////////////////
// API
/////////////////////////////////////

bool ISerializer::encode( const Schema &schema, const RuntimeObject &object,
                         std::vector<uint8_t> &outBuffer, SerializeFormat fmt) noexcept
{
    bool ret = false;

    do {
        if (!schema.isValid()) {
            JOB_LOG_ERROR("[serializer] invalid schema, cannot encode");
            break;
        }

        switch (fmt) {
        case SerializeFormat::Json:
            ret = encodeJson(schema, object, outBuffer);
            break;
        case SerializeFormat::Binary:
            ret = encodeBinary(schema, object, outBuffer);
            break;
        case SerializeFormat::Yaml:
            ret = encodeYaml(schema, object, outBuffer);
            break;
        case SerializeFormat::Text:
            ret = encodeText(schema, object, outBuffer);
            break;
        default:
            JOB_LOG_ERROR("[serializer] unknown format {}", static_cast<int>(fmt));
            break;
        }

    } while (0);

    return ret;
}

bool ISerializer::decode(const Schema &schema, RuntimeObject &outObject,
                         const std::vector<uint8_t> &inBuffer, SerializeFormat fmt) noexcept
{
    bool ret = false;

    do {
        if (!schema.isValid()) {
            JOB_LOG_ERROR("[serializer] invalid schema, cannot decode");
            break;
        }

        if (inBuffer.empty()) {
            JOB_LOG_ERROR("[serializer] input buffer is empty");
            break;
        }

        switch (fmt) {
        case SerializeFormat::Json:
            ret = decodeJson(schema, outObject, inBuffer);
            break;
        case SerializeFormat::Binary:
            ret = decodeBinary(schema, outObject, inBuffer);
            break;
        case SerializeFormat::Yaml:
            ret = decodeYaml(schema, outObject, inBuffer);
            break;
        case SerializeFormat::Text:
            ret = decodeText(schema, outObject, inBuffer);
            break;
        default:
            JOB_LOG_ERROR("[serializer] unknown format {}", static_cast<int>(fmt));
            break;
        }

    } while (0);

    return ret;
}

bool ISerializer::encodeJson( const Schema &schema, const RuntimeObject &object, std::vector<uint8_t> &outBuffer) noexcept
{
    bool ret = false;
    do {
        try {
            json j;
            // Iterate the SCHEMA, not the object
            for (const auto& field : schema.fields) {
                auto opt_val = object.getField(field.name);
                if (!opt_val) {
                    if (field.required) {
                        JOB_LOG_WARN("[serializer] JSON encode: missing required field '{}'", field.name);
                        j[field.name] = nullptr;
                    }
                    continue;
                }

                json j_val;
                if (!fieldValueToJson(*opt_val, j_val)) {
                    JOB_LOG_ERROR("[serializer] JSON encode: failed to convert field '{}'", field.name);
                    continue;
                }
                j[field.name] = j_val;
            }

            const auto dump = j.dump(2);
            outBuffer.assign(dump.begin(), dump.end());
            ret = true;
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[serializer] JSON encode error: {}", e.what());
        }

    } while (0);

    return ret;
}

bool ISerializer::decodeJson( const Schema &schema, RuntimeObject &outObject, const std::vector<uint8_t> &inBuffer) noexcept
{
    bool ret = false;

    do {
        try {
            if (inBuffer.empty()) {
                JOB_LOG_ERROR("[serializer] empty JSON buffer");
                break;
            }

            std::string str(inBuffer.begin(), inBuffer.end());
            json j = json::parse(str);

            if (!j.is_object()) {
                JOB_LOG_ERROR("[serializer] JSON root is not an object");
                break;
            }

            outObject.clear();

            // Iterate the SCHEMA, not the JSON object
            for (const auto& field : schema.fields) {
                if (!j.contains(field.name)) {
                    if (field.required)
                        JOB_LOG_WARN("[serializer] JSON decode: missing required field '{}'", field.name);
                    continue;
                }

                FieldValue fv;
                if (!jsonToFieldValue_DumbRecursive(j[field.name], fv)) {
                    JOB_LOG_ERROR("[serializer] JSON decode: failed to parse field '{}'", field.name);
                    continue;
                }
                outObject.setField(field.name, fv);
            }
            ret = true;
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[serializer] JSON decode error: {}", e.what());
        }
    } while (0);

    return ret;
}

bool ISerializer::encodeYaml( const Schema &schema, const RuntimeObject &object, std::vector<uint8_t> &outBuffer) noexcept
{
    bool ret = false;
    do {
        try {
            YAML::Emitter emitter;
            emitter << YAML::BeginMap;

            for (const auto& field : schema.fields) {
                auto opt_val = object.getField(field.name);
                if (!opt_val) {
                    if (field.required) {
                        JOB_LOG_WARN("[serializer] YAML encode: missing required field '{}'", field.name);
                        emitter << YAML::Key << field.name << YAML::Value << YAML::Null;
                    }
                    continue;
                }

                emitter << YAML::Key << field.name;
                emitter << YAML::Value;
                if (!fieldValueToYaml(*opt_val, emitter)) {
                    JOB_LOG_ERROR("[serializer] YAML encode: failed to convert field '{}'", field.name);
                    continue;
                }
            }

            emitter << YAML::EndMap;
            if (!emitter.good()) {
                JOB_LOG_ERROR("[serializer] YAML emitter is in a bad state after encoding");
                break;
            }

            std::string str = emitter.c_str();
            outBuffer.assign(str.begin(), str.end());
            ret = true;
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[serializer] YAML encode error: {}", e.what());
        }
    } while (0);

    return ret;
}

bool ISerializer::decodeYaml( const Schema &schema, RuntimeObject &outObject, const std::vector<uint8_t> &inBuffer) noexcept
{
    bool ret = false;
    do {
        try {
            if (inBuffer.empty()) {
                JOB_LOG_ERROR("[serializer] empty YAML buffer");
                break;
            }

            std::string str(inBuffer.begin(), inBuffer.end());
            YAML::Node root = YAML::Load(str);

            if (!root.IsMap()) {
                JOB_LOG_ERROR("[serializer] YAML root is not a map");
                break;
            }

            outObject.clear();
            for (const auto& field : schema.fields) {
                if (!root[field.name]) {
                    if (field.required)
                        JOB_LOG_WARN("[serializer] YAML decode: missing required field '{}'", field.name);
                    continue;
                }

                FieldValue fv;
                if (!yamlToFieldValue_DumbRecursive(root[field.name], fv)) {
                    JOB_LOG_ERROR("[serializer] YAML decode: failed to parse field '{}'", field.name);
                    continue;
                }

                outObject.setField(field.name, fv);
            }

            ret = true;
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[serializer] YAML decode error: {}", e.what());
        }
    } while (0);

    return ret;
}


bool ISerializer::encodeBinary( [[maybe_unused]] const Schema &schema, [[maybe_unused]] const RuntimeObject &object, [[maybe_unused]] std::vector<uint8_t> &outBuffer) noexcept
{
    JOB_LOG_ERROR("[serializer] encodeBinary is not implemented in the base class");
    return false;
}

bool ISerializer::decodeBinary( [[maybe_unused]] const Schema &schema, [[maybe_unused]] RuntimeObject &outObject, [[maybe_unused]] const std::vector<uint8_t> &inBuffer) noexcept
{
    JOB_LOG_ERROR("[serializer] decodeBinary is not implemented in the base class");
    return false;
}

bool ISerializer::encodeText( [[maybe_unused]] const Schema &schema, [[maybe_unused]] const RuntimeObject &object, [[maybe_unused]] std::vector<uint8_t> &outBuffer) noexcept
{
    JOB_LOG_ERROR("[serializer] encodeText is not implemented in the base class");
    return false;
}

bool ISerializer::decodeText( [[maybe_unused]] const Schema &schema, [[maybe_unused]] RuntimeObject &outObject, [[maybe_unused]] const std::vector<uint8_t> &inBuffer) noexcept
{
    JOB_LOG_ERROR("[serializer] decodeText is not implemented in the base class");
    return false;
}

} // namespace job::serializer

