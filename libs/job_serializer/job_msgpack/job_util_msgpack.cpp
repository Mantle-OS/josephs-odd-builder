#include "job_util_msgpack.h"

#include <job_logger.h>

namespace job::serializer::msg_pack {

std::string JobUtilMsgPack::getCppType(const Field &f)
{
    switch (f.kind) {
    case FieldKind::Scalar:
        return f.type;
    case FieldKind::Bin:
        if (f.size)
            return "std::array<uint8_t, " + std::to_string(*f.size) + ">";
        return "std::vector<uint8_t>";
    case FieldKind::ListScalar:
        return "std::vector<" + f.type.substr(5, f.type.size() - 6) + ">";
    case FieldKind::ListBin:
        if (f.size)
            return "std::vector<std::array<uint8_t, " + std::to_string(*f.size) + "> >";
        return "std::vector<std::vector<uint8_t> >";
    case FieldKind::Struct:
        return f.ref_sym.value_or("UNKNOWN_STRUCT");
    case FieldKind::ListStruct:
        return "std::vector<" + f.ref_sym.value_or("UNKNOWN_STRUCT") + ">";
    }

    JOB_LOG_WARN( "[JobUtilMsgPack] Could not gather the CPP type for {} This is not good .... not good at all", static_cast<int>(f.kind));
    return "void"; // Should not happen
}

std::string JobUtilMsgPack::getPackFunc(const Field &f)
{
    std::ostringstream ss;
    ss << "        // Pack field: " << f.name << "\n";
    ss << "        pk.pack_str(" << f.name.size() << ");\n";
    ss << "        pk.pack_str_body(\"" << f.name << "\", " << f.name.size() << ");\n";
    if (f.kind == FieldKind::ListStruct || f.kind == FieldKind::Struct)
        ss << "        " << f.name << ".pack_msgpack(pk);\n";
    else
        ss << "        pk.pack(" << f.name << ");\n";


    return ss.str();
}

std::string JobUtilMsgPack::getUnpackFunc(const Field &f)
{
    std::ostringstream ss;
    ss << "        // Unpack field: " << f.name << "\n";
    ss << "        if (key == \"" << f.name << "\") {\n";

    if (f.kind == FieldKind::ListStruct || f.kind == FieldKind::Struct)
        ss << "            " << f.name << ".unpack_msgpack(val_obj);\n";
    else
        ss << "            val_obj.convert(" << f.name << ");\n";

    ss << "        }";
    return ss.str();
}

bool JobUtilMsgPack::packFieldValue(const FieldValue &fv, msgpack::packer<msgpack::sbuffer> &pk) noexcept
{
    if (fv.isScalar()) {
        std::visit(ScalarPackVisitor{pk}, std::get<FieldValue::Scalar>(fv.value));
    } else if (fv.isBinary()) {
        const auto &bin = std::get<FieldValue::Binary>(fv.value);
        pk.pack_bin(bin.size());
        pk.pack_bin_body(reinterpret_cast<const char *>(bin.data()), bin.size());
    } else if (fv.isList()) {
        const auto &list = std::get<FieldValue::List>(fv.value);
        pk.pack_array(list.size());
        for (const auto &item : list)
            if (!packFieldValue(item, pk))
                return false;
    } else if (fv.isStruct()) {
        const auto &map = std::get<FieldValue::Struct>(fv.value);
        pk.pack_map(map.size());
        for (const auto &[key, val] : map) {
            pk.pack_str(key.size());
            pk.pack_str_body(key.data(), key.size());
            if (!packFieldValue(val, pk))
                return false;
        }
    } else {
        pk.pack_nil();
    }
    return true;
}

bool JobUtilMsgPack::unpackFieldValue(const msgpack::object &obj, FieldValue &out_fv) noexcept
{
    switch (obj.type) {
    case msgpack::type::NIL:
        out_fv.value = std::monostate{};
        break;
    case msgpack::type::NEGATIVE_INTEGER:
        out_fv.value = FieldValue::Scalar{obj.as<int64_t>()};
        break;
    case msgpack::type::POSITIVE_INTEGER:
        out_fv.value = FieldValue::Scalar{obj.as<uint64_t>()};
        break;
    case msgpack::type::STR:
        out_fv.value = FieldValue::Scalar{obj.as<std::string>()};
        break;
    case msgpack::type::BIN:
        out_fv.value = FieldValue::Binary(obj.via.bin.ptr, obj.via.bin.ptr + obj.via.bin.size);
        break;
    case msgpack::type::ARRAY: {
        FieldValue::List list;
        list.reserve(obj.via.array.size);
        for (uint32_t i = 0; i < obj.via.array.size; ++i) {
            FieldValue item;
            if (!unpackFieldValue(obj.via.array.ptr[i], item))
                return false;
            list.push_back(std::move(item));
        }
        out_fv.value = std::move(list);
        break;
    }
    case msgpack::type::MAP: {
        FieldValue::Struct map;
        for (uint32_t i = 0; i < obj.via.map.size; ++i) {
            const msgpack::object_kv &kv = obj.via.map.ptr[i];
            std::string key = kv.key.as<std::string>();
            FieldValue val;
            if (!unpackFieldValue(kv.val, val))
                return false;
            map[std::move(key)] = std::move(val);
        }
        out_fv.value = std::move(map);
        break;
    }
    default:
        JOB_LOG_WARN("[msgpack] Unsupported msgpack type: {}",  static_cast<int>(obj.type));
        return false;
    }
    return true;
}


}

