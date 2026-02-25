#pragma once

#include <msgpack.hpp>

#include <job_serializer_utils.h>
#include <job_field.h>

namespace job::serializer::msg_pack {
using namespace job::serializer;

struct ScalarPackVisitor
{
    msgpack::packer<msgpack::sbuffer> &pk;

    ScalarPackVisitor(msgpack::packer<msgpack::sbuffer> &packer) :
        pk(packer)
    {}

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
    void operator()(const std::string &val) const
    {
        pk.pack_str(val.size());
        pk.pack_str_body(val.data(), val.size());
    }
};


class JobUtilMsgPack final {

public:
    JobUtilMsgPack() = default;
    ~JobUtilMsgPack() = delete;

    // JobUtilMsgPack(const JobUtilMsgPack &) = default;
    // JobUtilMsgPack &operator=(const JobUtilMsgPack &) = delete;

    [[nodiscard]] static std::string getCppType(const Field &f);
    [[nodiscard]] static std::string getPackFunc(const Field &f);
    [[nodiscard]] static std::string getUnpackFunc(const Field &f);

    [[nodiscard]] static bool packFieldValue( const FieldValue &fv, msgpack::packer<msgpack::sbuffer> &pk) noexcept;
    [[nodiscard]] static bool unpackFieldValue( const msgpack::object &obj, FieldValue &out_fv) noexcept;
};
}

