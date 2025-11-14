#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <string>
#include <fstream>

#include <job_logger.h>
#include <runtime_object.h>
#include <serializer.h>
#include <schema.h>

using namespace job::msgpack;
using namespace job::core;

static std::filesystem::path makeTempSchema()
{
    auto tmp = std::filesystem::temp_directory_path() / "job_schema_test.yaml";
    std::ofstream out(tmp);

    out << R"(
tag: test_tag
version: 1
unit: core
base: basefile
c_struct: test_struct
include_prefix: include/jobmsgpack
out_base: test_out
fields:
  - key: 1
    name: field_str
    type: str
    required: true
    comment: "Sample string field"
  - key: 2
    name: field_bin
    type: bin[16]
  - key: 3
    name: field_list
    type: list<str>
)";
    out.close();
    return tmp;
}
TEST_CASE("Serialization and Deserialization consistency MSGPACK", "[serializer][encoding][decoding]")
{
    // Load the schema from the temp YAML file
    auto schemaPath = makeTempSchema();
    Schema schema;

    // Load schema and ensure it's valid
    REQUIRE(loadSchemaFromFile(schemaPath, schema));
    REQUIRE(schema.isValid());

    RuntimeObject object;
    Serializer serializer;

    // Set up field values
    FieldValue nameField;
    nameField.value = FieldValue::Scalar{"test_name"};
    object.setField("name", nameField);

    FieldValue binaryField;
    binaryField.value = FieldValue::Binary{std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04}};
    object.setField("binary_data", binaryField);

    FieldValue listField;
    FieldValue::List list = {FieldValue{FieldValue::Scalar{"item1"}}, FieldValue{FieldValue::Scalar{"item2"}}};
    listField.value = list;
    object.setField("list_data", listField);

    std::vector<uint8_t> msgpackBuffer;

    // Encoding test
    REQUIRE(serializer.encode(schema, object, msgpackBuffer, SerializeFormat::MsgPack));
    REQUIRE_FALSE(msgpackBuffer.empty());
    JOB_LOG_INFO("[test] MsgPack buffer size: {}", msgpackBuffer.size());

    // Decoding test
    RuntimeObject decodedObjectFromMsgPack;
    REQUIRE(serializer.decode(schema, decodedObjectFromMsgPack, msgpackBuffer, SerializeFormat::MsgPack));

    auto nameFieldDecoded = decodedObjectFromMsgPack.getField("name");
    auto binaryFieldDecoded = decodedObjectFromMsgPack.getField("binary_data");
    auto listFieldDecoded = decodedObjectFromMsgPack.getField("list_data");

    REQUIRE(nameFieldDecoded.has_value());
    REQUIRE(binaryFieldDecoded.has_value());
    REQUIRE(listFieldDecoded.has_value());

    // Check 'name' field
    const auto& scalar = std::get<FieldValue::Scalar>(nameFieldDecoded->value);
    REQUIRE(std::holds_alternative<std::string>(scalar));
    REQUIRE(std::get<std::string>(scalar) == "test_name");

    // Check 'binary_data' field
    const auto& binaryData = std::get<FieldValue::Binary>(binaryFieldDecoded->value);
    REQUIRE(binaryData == std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04});

    // Check 'list_data' field
    const auto& item1 = std::get<FieldValue::List>(listFieldDecoded->value);
    REQUIRE(item1.size() == 2);

    // Validate list items
    REQUIRE(std::holds_alternative<FieldValue::Scalar>(item1[0].value));
    REQUIRE(std::get<std::string>(std::get<FieldValue::Scalar>(item1[0].value)) == "item1");

    REQUIRE(std::holds_alternative<FieldValue::Scalar>(item1[1].value));
    REQUIRE(std::get<std::string>(std::get<FieldValue::Scalar>(item1[1].value)) == "item2");

    // Clean up the temporary schema file
    std::filesystem::remove(schemaPath);
}
// CHECKPOINT: v1
