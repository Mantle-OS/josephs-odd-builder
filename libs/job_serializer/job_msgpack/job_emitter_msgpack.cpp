#include "job_emitter_msgpack.h"
#include "job_util_msgpack.h"

#include <sstream>

#include <job_logger.h>

namespace job::serializer::msg_pack {

std::string JobEmitterMsgPack::appendDecl([[maybe_unused]] const Schema &schema ) noexcept
{
    std::ostringstream ss;
    ss << "\n";
    ss << "    void pack_msgpack(msgpack::packer<msgpack::sbuffer> &pk) const;\n";
    ss << "    void unpack_msgpack(const msgpack::object &obj);\n";
    return ss.str();
}

std::string JobEmitterMsgPack::appendImply( const Schema &schema ) noexcept
{
    std::ostringstream ss;
    ss << "\n// --- Appended by JobEmitterMsgPack (Source) --- \n";

    ss << "void " << schema.c_struct << "::pack_msgpack(msgpack::packer<msgpack::sbuffer> &pk) const {\n";
    ss << "    pk.pack_map(" << schema.fields.size() << ");\n";
    for (const auto &f : schema.fields)
        ss << JobUtilMsgPack::getPackFunc(f);
    ss << "}\n\n";

    ss << "void " << schema.c_struct << "::unpack_msgpack(const msgpack::object &obj) {\n";
    ss << "    if (obj.type != msgpack::type::MAP) return;\n";
    ss << "    for (uint32_t i = 0; i < obj.via.map.size; ++i) {\n";
    ss << "        const msgpack::object_kv &kv = obj.via.map.ptr[i];\n";
    ss << "        const std::string key = kv.key.as<std::string>();\n";
    ss << "        const msgpack::object &val_obj = kv.val;\n\n";

    bool first = true;
    for (const auto &f : schema.fields) {
        if (!first) ss << " else ";
        ss << JobUtilMsgPack::getUnpackFunc(f);
        first = false;
    }

    ss << "\n    }\n";
    ss << "}\n";

    return ss.str();
}

} // namespace job::serializer::msg_pack

