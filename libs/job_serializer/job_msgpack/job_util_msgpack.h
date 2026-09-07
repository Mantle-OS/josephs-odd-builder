#pragma once

#include <cstdint>
#include <string>

#include <msgpack.hpp>

#include <job_field.h>
#include <job_serializer_utils.h>

#include "jobserializermsgpack_export.h"

namespace job::serializer::msg_pack {

struct ScalarPackVisitor
{
    explicit ScalarPackVisitor(msgpack::packer<msgpack::sbuffer> &packer) :
        pk(packer)
    {
    }

    void operator()(const int32_t &val) const
    {
        pk.pack_int32(val);
    }

    void operator()(const uint32_t &val) const
    {
        pk.pack_uint32(val);
    }

    void operator()(const int64_t &val) const
    {
        pk.pack_int64(val);
    }

    void operator()(const uint64_t &val) const
    {
        pk.pack_uint64(val);
    }

    void operator()(const bool &val) const
    {
        pk.pack(val);
    }

    void operator()(const float &val) const
    {
        pk.pack(val);
    }

    void operator()(const double &val) const
    {
        pk.pack(val);
    }

    void operator()(const std::string &val) const
    {
        pk.pack_str(val.size());
        pk.pack_str_body(val.data(), val.size());
    }

    msgpack::packer<msgpack::sbuffer> &pk;
};

class JOBSERIALIZERMSGPACK_EXPORT JobUtilMsgPack final
{
public:
    JobUtilMsgPack() = delete;
    ~JobUtilMsgPack() = delete;

    JobUtilMsgPack(const JobUtilMsgPack &) = delete;
    JobUtilMsgPack &operator=(const JobUtilMsgPack &) = delete;
    JobUtilMsgPack(JobUtilMsgPack &&) = delete;
    JobUtilMsgPack &operator=(JobUtilMsgPack &&) = delete;

    [[nodiscard]] static std::string getCppType(const Field &f);
    [[nodiscard]] static std::string getPackFunc(const Field &f);
    [[nodiscard]] static std::string getUnpackFunc(const Field &f);

    [[nodiscard]] static bool packFieldValue(const FieldValue &fv,
                                             msgpack::packer<msgpack::sbuffer> &pk) noexcept;
    [[nodiscard]] static bool unpackFieldValue(const msgpack::object &obj,
                                               FieldValue &out_fv) noexcept;
};

}