#pragma once

#include <job_serializer_utils.h>
#include <schema.h>
#include <emitters/cpp_emitter.h>

using namespace job::serializer;
class TestEmitter final : public CppEmitter
{
public:
    [[nodiscard]] std::string appendDecl( const Schema &schema) noexcept override
    {
        std::ostringstream ss;
        ss << "\n    // --- Appended Header (TestEmitter) --- \n";
        ss << "    void print_" << schema.out_base << "(const " << schema.c_struct << " &obj);\n";
        return ss.str();
    }

    [[nodiscard]] std::string appendImply( const Schema &schema) noexcept override
    {
        std::ostringstream ss;
        ss << "\n// --- Appended Source (TestEmitter) --- \n";
        ss << "void print_" << schema.out_base << "(const " << schema.c_struct << " &obj)\n";
        ss << "{\n";
        ss << "    std::cout << \"" << schema.c_struct << "{\\n\";\n";

        // This is the implementation for our generated print function
        for (const auto &f : schema.fields)
            ss << "    std::cout << \"  " << f.name << ": \" << /* TODO: serialize field */ \"...\" << \"\\n\";\n";

        ss << "    std::cout << \"}\\n\";\n";
        ss << "}\n";
        return ss.str();
    }

    static Schema getEmitterTestSchema()
    {
        Schema s{};
        s.tag = "EmitterTest";
        s.version = 1;
        s.unit = "tests";
        s.base = "BaseStruct";
        s.c_struct = "EmitterTest_t";
        s.include_prefix = "job_tests";
        s.out_base = "emitter_test_msg";

        s.hdr_name = s.out_base + ".hpp";
        s.src_name = s.out_base + ".cpp";

        //  Scalar
        s.fields.push_back({
            .key = 1,
            .name = "count",
            .type = "u32",
            .kind = FieldKind::Scalar,
            .size{},
            .ctype{},
            .ref_include{},
            .ref_sym{},
            .required = false,
            .comment{}
        });

        // Fixed-size Binary
        s.fields.push_back({
            .key = 2,
            .name = "data",
            .type = "bin[32]",
            .kind = FieldKind::Bin,
            .size = 32,
            .ctype{},
            .ref_include{},
            .ref_sym{},
            .required = false,
            .comment{}
        });

        // List of scalars
        s.fields.push_back({
            .key = 3,
            .name = "names",
            .type = "list<str>",
            .kind = FieldKind::ListScalar,
            .size{},
            .ctype{},
            .ref_include{},
            .ref_sym{},
            .required = false,
            .comment{}
        });

        // List of fixed-size binary
        s.fields.push_back({
            .key = 4,
            .name = "packets",
            .type = "list<bin[128]>",
            .kind = FieldKind::ListBin,
            .size = 128,
            .ctype{},
            .ref_include{},
            .ref_sym{},
            .required = false,
            .comment{}
        });

        // Nested Struct
        s.fields.push_back({
            .key = 5,
            .name = "header",
            .type = "struct",
            .kind = FieldKind::Struct,
            .size{},
            .ctype{},
            .ref_include = "my_header.hpp",
            .ref_sym = "MyHeader",
            .required = false,
            .comment{}
        });

        // List of Nested Structs
        s.fields.push_back({
            .key = 6,
            .name = "items",
            .type = "list<struct>",
            .kind = FieldKind::ListStruct,
            .size{},
            .ctype{},
            .ref_include = "my_item.hpp",
            .ref_sym = "MyTestItem",
            .required = false,
            .comment{}
        });
        return s;
    }
};

static inline bool stringContains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

