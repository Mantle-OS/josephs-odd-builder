#include "job_serializer_utils.h"
#include "job_serializer_logger.h"
namespace job::serializer {
FieldKind deduceFieldKind(const std::string &type) noexcept
{
    FieldKind ret = FieldKind::Scalar;

    if (type == "str"    ||
        type == "bool"   ||
        type == "float"  ||
        type == "double" ||
        type == "i32"    ||
        type == "u32"    ||
        type == "i64"    ||
        type == "u64") {
        ret = FieldKind::Scalar;
    } else if (type == "bin" || type.starts_with("bin[")) {
        ret = FieldKind::Bin;
    } else if (type == "struct") {
        ret = FieldKind::Struct;
    } else if (type == "list<str>") {
        ret = FieldKind::ListScalar;
    } else if (type == "list<bin>" || type.starts_with("list<bin[")) {
        ret = FieldKind::ListBin;
    } else if (type == "list<struct>") {
        ret = FieldKind::ListStruct;
    } else {
        // if language built in type this can trip this up
        // example a bool or a float or a double
        JOB_SER_WARN("[schema] Unknown field type: {}", type);
    }

    return ret;
}

} // namespace job::serializer

