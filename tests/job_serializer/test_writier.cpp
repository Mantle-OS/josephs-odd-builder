#include <catch2/catch_all.hpp>

#include <cstdint>
#include <string>

#include <job_logger.h>
#include <job_tmp_file.h>
#include <iserializer.h>
#include <reader.h>
#include <runtime_object.h>
#include <schema.h>
#include <writer.h>

#include "test_emitter.h"

using namespace job::serializer;

TEST_CASE("Writer::writeSchema round-trip", "[job_writer]")
{
    Schema sIn = TestEmitter::getEmitterTestSchema();

    SECTION("YAML round-trip")
    {
        job::io::JobTmpFile tmpFile("test_writer_schema.yaml");

        Writer writer(tmpFile.path());
        REQUIRE(writer.writeSchema(sIn));

        Schema sOut{};
        Reader reader(tmpFile.path());

        REQUIRE(reader.readSchema(sOut));
        REQUIRE(sIn == sOut);
    }

    SECTION("JSON round-trip")
    {
        job::io::JobTmpFile tmpFile("test_writer_schema.json");

        Writer writer(tmpFile.path());
        REQUIRE(writer.writeSchema(sIn));

        Schema sOut{};
        Reader reader(tmpFile.path());

        REQUIRE(reader.readSchema(sOut));
        REQUIRE(sIn == sOut);
    }

    SECTION("Write failure on invalid schema")
    {
        job::io::JobTmpFile tmpFile("invalid_schema.yaml");

        Schema invalidSchema{};

        REQUIRE_FALSE(invalidSchema.isValid());

        Writer writer(tmpFile.path());

        REQUIRE_FALSE(writer.writeSchema(invalidSchema));
    }
}

TEST_CASE("Writer::writeEmitter writes generated code to files", "[job_writer]")
{
    Schema s = TestEmitter::getEmitterTestSchema();
    TestEmitter emitter{};

    job::io::JobTmpFile headerFile(s.out_base + ".hpp");
    job::io::JobTmpFile sourceFile(s.out_base + ".cpp");

    auto [expectedHeader, expectedSource] = emitter.render(s);

    REQUIRE_FALSE(expectedHeader.empty());
    REQUIRE_FALSE(expectedSource.empty());

    Writer writer("emitter_writer.tmp");

    REQUIRE(writer.writeEmitter(emitter,
                                s,
                                headerFile.path(),
                                sourceFile.path()));

    Reader headerReader(headerFile.path());
    Reader sourceReader(sourceFile.path());

    const std::string headerContent = headerReader.readAll();
    const std::string sourceContent = sourceReader.readAll();

    REQUIRE(headerContent == expectedHeader);
    REQUIRE(sourceContent == expectedSource);
}

TEST_CASE("Writer::writeRuntime round-trip (JSON)", "[job_writer]")
{
    Schema s = TestEmitter::getEmitterTestSchema();
    ISerializer ser{};
    RuntimeObject objIn{};

    objIn.setField("count", FieldValue{
                                .value = FieldValue::Scalar{static_cast<uint32_t>(42)}
                            });

    objIn.setField("data", FieldValue{
                               .value = FieldValue::Scalar{std::string("test_data")}
                           });

    job::io::JobTmpFile tmpFile("test_writer_runtime.json");

    Writer writer(tmpFile.path());

    REQUIRE(writer.writeRuntime(ser, s, objIn, SerializeFormat::Json));

    RuntimeObject objOut{};
    Reader reader(tmpFile.path());

    REQUIRE(reader.readRuntime(ser, s, objOut, SerializeFormat::Json));
    REQUIRE(objOut.hasField("count"));
    REQUIRE(objOut.hasField("data"));

    auto countVal = objOut.getField("count");
    auto dataVal = objOut.getField("data");

    REQUIRE(countVal.has_value());
    REQUIRE(dataVal.has_value());

    REQUIRE(std::get<FieldValue::Scalar>(countVal->value) ==
            FieldValue::Scalar{static_cast<int64_t>(42)});

    REQUIRE(std::get<FieldValue::Scalar>(dataVal->value) ==
            FieldValue::Scalar{std::string("test_data")});
}