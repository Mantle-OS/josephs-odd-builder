#include "job_util_msgpack.h"

#include <job_serializer_logger.h>

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

    JOB_SER_WARN( "[JobUtilMsgPack] Could not gather the CPP type for {} This is not good .... not good at all", static_cast<int>(f.kind));
    return "void"; // Should not happen
}

std::string JobUtilMsgPack::getPackFunc(const Field &f)
{
    std::ostringstream ss;
    ss << "        // Pack field: " << f.name << "\n";
    ss << "        pk.pack_str(" << f.name.size() << ");\n";
    ss << "        pk.pack_str_body(\"" << f.name << "\", " << f.name.size() << ");\n";

    if (f.kind == FieldKind::ListStruct) {
        ss << "        pk.pack_array(static_cast<uint32_t>(" << f.name << ".size()));\n";
        ss << "        for (const auto& item : " << f.name << ") {\n";
        ss << "            item.pack_msgpack(pk);\n";
        ss << "        }\n";
    } else if (f.kind == FieldKind::Struct) {
        ss << "        " << f.name << ".pack_msgpack(pk);\n";
    } else if (f.kind == FieldKind::Bin) {
        ss << "        pk.pack_bin(static_cast<uint32_t>(" << f.name << ".size()));\n";
        ss << "        pk.pack_bin_body(reinterpret_cast<const char*>(" << f.name << ".data()), static_cast<uint32_t>(" << f.name << ".size()));\n";
    } else if (f.kind == FieldKind::ListBin) {
        ss << "        pk.pack_array(static_cast<uint32_t>(" << f.name << ".size()));\n";
        ss << "        for (const auto& item : " << f.name << ") {\n";
        ss << "            pk.pack_bin(static_cast<uint32_t>(item.size()));\n";
        ss << "            pk.pack_bin_body(reinterpret_cast<const char*>(item.data()), static_cast<uint32_t>(item.size()));\n";
        ss << "        }\n";
    } else {
        ss << "        pk.pack(" << f.name << ");\n";
    }

    return ss.str();
}

std::string JobUtilMsgPack::getUnpackFunc(const Field &f)
{
    std::ostringstream ss;
    ss << "        // Unpack field: " << f.name << "\n";
    ss << "        if (key == \"" << f.name << "\") {\n";

    if (f.kind == FieldKind::ListStruct) {
        ss << "            if (val_obj.type == msgpack::type::ARRAY) {\n";
        ss << "                " << f.name << ".clear();\n";
        ss << "                " << f.name << ".reserve(val_obj.via.array.size);\n";
        ss << "                for (uint32_t i = 0; i < val_obj.via.array.size; ++i) {\n";
        ss << "                    " << f.ref_sym.value() << " item;\n";
        ss << "                    item.unpack_msgpack(val_obj.via.array.ptr[i]);\n";
        ss << "                    " << f.name << ".push_back(item);\n";
        ss << "                }\n";
        ss << "            }\n";
    } else if (f.kind == FieldKind::Struct) {
        ss << "            " << f.name << ".unpack_msgpack(val_obj);\n";
    } else if (f.kind == FieldKind::Bin) {
        ss << "            if (val_obj.type == msgpack::type::BIN) {\n";
        if (f.size) {
            ss << "                if (val_obj.via.bin.size != " << *f.size << ")\n";
            ss << "                    throw std::runtime_error(\"size mismatch unpacking bin field '" << f.name << "', expected " << *f.size << " bytes, got \" + std::to_string(val_obj.via.bin.size));\n";
            ss << "                std::memcpy(" << f.name << ".data(), val_obj.via.bin.ptr, " << *f.size << ");\n";
        } else {
            ss << "                " << f.name << ".assign(val_obj.via.bin.ptr, val_obj.via.bin.ptr + val_obj.via.bin.size);\n";
        }
        ss << "            }\n";
    } else if (f.kind == FieldKind::ListBin) {
        ss << "            if (val_obj.type == msgpack::type::ARRAY) {\n";
        ss << "                " << f.name << ".clear();\n";
        ss << "                " << f.name << ".reserve(val_obj.via.array.size);\n";
        ss << "                for (uint32_t i = 0; i < val_obj.via.array.size; ++i) {\n";
        ss << "                    const auto &elem = val_obj.via.array.ptr[i];\n";
        ss << "                    if (elem.type != msgpack::type::BIN) continue;\n";
        if (f.size) {
            ss << "                    if (elem.via.bin.size != " << *f.size << ")\n";
            ss << "                        throw std::runtime_error(\"size mismatch unpacking list<bin> element in '" << f.name << "', expected " << *f.size << " bytes, got \" + std::to_string(elem.via.bin.size));\n";
            ss << "                    std::array<uint8_t, " << *f.size << "> item{};\n";
            ss << "                    std::memcpy(item.data(), elem.via.bin.ptr, " << *f.size << ");\n";
        } else {
            ss << "                    std::vector<uint8_t> item(elem.via.bin.ptr, elem.via.bin.ptr + elem.via.bin.size);\n";
        }
        ss << "                    " << f.name << ".push_back(std::move(item));\n";
        ss << "                }\n";
        ss << "            }\n";
    } else {
        ss << "            val_obj.convert(" << f.name << ");\n";
    }
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
    case msgpack::type::BOOLEAN:
        out_fv.value = FieldValue::Scalar{obj.as<bool>()};
        break;
    case msgpack::type::FLOAT32:
        out_fv.value = FieldValue::Scalar{obj.as<float>()};
        break;
    case msgpack::type::FLOAT64:
        out_fv.value = FieldValue::Scalar{obj.as<double>()};
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
        JOB_SER_WARN("[msgpack] Unsupported msgpack type: {}",  static_cast<int>(obj.type));
        return false;
    }
    return true;
}


}

