#include <catch2/catch_all.hpp>

#include <string>
#include <filesystem>


#include <job_logger.h>
#include <schema.h>
#include <reader.h>
#include <runtime_object.h>
#include <iserializer.h>

#include <writer.h>
#include "test_emitter.h"

using namespace job::serializer;
namespace fs = std::filesystem;

TEST_CASE("Writer::writeSchema round-trip", "[job_writer]"){
    Schema s_in = TestEmitter::getEmitterTestSchema();
    const fs::path yamlPath = "test_writer_schema.yaml";
    const fs::path jsonPath = "test_writer_schema.json";

    fs::remove(yamlPath);
    fs::remove(jsonPath);

    SECTION("YAML round-trip"){
        {
            Writer w(yamlPath);
            REQUIRE(w.writeSchema(s_in)); // Let writeSchema auto-detect format
            w.closeDevice();
            w.flush();
        }
        REQUIRE(fs::exists(yamlPath));

        Schema s_out{};
        Reader r(yamlPath);
        REQUIRE(r.readSchema(s_out)); // Let readSchema auto-detect format

        REQUIRE(s_in == s_out);
    }

    SECTION("JSON round-trip") {
        {
            Writer w(jsonPath);
            REQUIRE(w.writeSchema(s_in));
        }
        REQUIRE(fs::exists(jsonPath));
        Schema s_out{};
        Reader r(jsonPath);
        REQUIRE(r.readSchema(s_out));
        REQUIRE(s_in == s_out);
    }

    SECTION("Write failure on invalid schema") {
        Schema s_invalid{};
        REQUIRE_FALSE(s_invalid.isValid());
        Writer w("invalid_schema.yaml");
        REQUIRE_FALSE(w.writeSchema(s_invalid));
    }

    fs::remove(yamlPath);
    fs::remove(jsonPath);
}

TEST_CASE("Writer::writeEmitter writes generated code to files", "[job_writer]"){
    Schema s = TestEmitter::getEmitterTestSchema();
    TestEmitter emitter{};

    const fs::path hdrPath = s.out_base + ".hpp";
    const fs::path srcPath = s.out_base + ".cpp";

    fs::remove(hdrPath);
    fs::remove(srcPath);

    auto [expectedHeader, expectedSource] = emitter.render(s);
    REQUIRE_FALSE(expectedHeader.empty());
    REQUIRE_FALSE(expectedSource.empty());
    {
        Writer w("emitter_writer.tmp");
        REQUIRE(w.writeEmitter(emitter, s, hdrPath, srcPath));
    }

    REQUIRE(fs::exists(hdrPath));
    REQUIRE(fs::exists(srcPath));
    std::string hdrContent;
    {
        Reader r(hdrPath);
        hdrContent = r.readAll();
    }

    std::string srcContent;
    {
        Reader r(srcPath);
        srcContent = r.readAll();
    }

    REQUIRE(hdrContent == expectedHeader);
    REQUIRE(srcContent == expectedSource);

    fs::remove(hdrPath);
    fs::remove(srcPath);
}

TEST_CASE("Writer::writeRuntime round-trip (JSON)", "[job_writer]") {
    Schema s = TestEmitter::getEmitterTestSchema();
    ISerializer ser{};
    RuntimeObject obj_in{}; // Use the real RuntimeObject

    obj_in.setField("count", FieldValue{ .value = FieldValue::Scalar{(uint32_t)42} });
    obj_in.setField("data", FieldValue{ .value = FieldValue::Scalar{std::string("test_data")} });

    const fs::path jsonPath = "test_writer_runtime.json";
    fs::remove(jsonPath);

    {
        Writer w(jsonPath);
        REQUIRE(w.writeRuntime(ser, s, obj_in, SerializeFormat::Json));
    }
    REQUIRE(fs::exists(jsonPath));

    RuntimeObject obj_out{};
    Reader r(jsonPath);

    REQUIRE(r.readRuntime(ser, s, obj_out, SerializeFormat::Json));
    REQUIRE(obj_out.hasField("count"));
    REQUIRE(obj_out.hasField("data"));

    auto countVal = obj_out.getField("count");
    auto dataVal = obj_out.getField("data");

    REQUIRE(std::get<FieldValue::Scalar>(countVal->value) == FieldValue::Scalar{(int64_t)42});
    REQUIRE(std::get<FieldValue::Scalar>(dataVal->value) == FieldValue::Scalar{std::string("test_data")});

    fs::remove(jsonPath);
}
