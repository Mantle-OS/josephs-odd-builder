#include "job_serializer_msgpack.h"
#include "job_util_msgpack.h"

#include <msgpack.hpp>
#include <job_logger.h>

namespace job::serializer::msg_pack {

[[nodiscard]] bool JobMsgPackSerializer::encodeBinary(const Schema &schema, const RuntimeObject &object, std::vector<uint8_t> &outBuffer) noexcept
{
    try {
        msgpack::sbuffer sbuf;
        msgpack::packer<msgpack::sbuffer> pk(&sbuf);
        pk.pack_map(schema.fields.size());

        for (const auto &f : schema.fields) {
            pk.pack_str(f.name.size());
            pk.pack_str_body(f.name.data(), f.name.size());

            auto fv_opt = object.getField(f.name);
            if (!fv_opt) {
                // Not found. Pack a nil.
                pk.pack_nil();
                continue;
            }
            if (!JobUtilMsgPack::packFieldValue(*fv_opt, pk)) {
                JOB_LOG_WARN("[msgpack] Failed to pack field: {}", f.name);
                pk.pack_nil();
            }
        }

        outBuffer.assign(sbuf.data(), sbuf.data() + sbuf.size());
        return true;

    } catch(const std::exception &e) {
        JOB_LOG_ERROR("[msgpack] encodeBinary error: {}", e.what());
        return false;
    }
}

bool JobMsgPackSerializer::decodeBinary(const Schema &schema, RuntimeObject &outObject, const std::vector<uint8_t> &inBuffer) noexcept
{
    try {
        if (inBuffer.empty()) {
            JOB_LOG_ERROR("[msgpack] decodeBinary buffer is empty");
            return false;
        }

        msgpack::object_handle oh = msgpack::unpack(reinterpret_cast<const char*>(inBuffer.data()), inBuffer.size());
        msgpack::object root = oh.get();

        if (root.type != msgpack::type::MAP) {
            JOB_LOG_ERROR("[msgpack] Root object is not a map");
            return false;
        }
        for (const auto &f : schema.fields) {
            msgpack::object val_obj;
            bool found = false;
            for (uint32_t i = 0; i < root.via.map.size; ++i) {
                const msgpack::object_kv &kv = root.via.map.ptr[i];
                if (kv.key.as<std::string>() == f.name) {
                    val_obj = kv.val;
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (f.required) {
                    JOB_LOG_ERROR("[msgpack] Missing required field: {}", f.name);
                    return false;
                }
                continue;
            }

            FieldValue fv;
            if (!JobUtilMsgPack::unpackFieldValue(val_obj, fv)) {
                JOB_LOG_WARN("[msgpack] Failed to unpack field: {}", f.name);
                continue;
            }

            outObject.setField(f.name, fv);
        }
        return true;

    } catch (const std::exception &e) {
        JOB_LOG_ERROR("[msgpack] decodeBinary error: {}", e.what());
        return false;
    }
}

} // namespace job::serializer::msg_pack
// CHECKPOINT: v1
