#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <chat/jinja_ast.h>
#include <chat/jinja_lexer.h>
#include <chat/jinja_parser.h>
#include <chat/jinja_token.h>
#include <chat/jinja_vm.h>

using job::token::BodyNode;
using job::token::JinjaLexer;
using job::token::JinjaParser;
using job::token::JinjaToken;
using job::token::JinjaVM;
using job::token::Value;
using job::token::ValueList;
using job::token::ValueMap;
using job::token::ValueType;

//
// Block 1: usage / examples
//

TEST_CASE("Value represents runtime scalar types", "[token][jinja][vm][value][usage]")
{
    SECTION("none")
    {
        const Value value;

        REQUIRE(value.type() == ValueType::None);
        REQUIRE(value.isNone());
        REQUIRE_FALSE(value.isTruthy());
        REQUIRE(value.toString().empty());
    }

    SECTION("bool")
    {
        const Value yes{true};
        const Value no{false};

        REQUIRE(yes.type() == ValueType::Bool);
        REQUIRE(yes.isBool());
        REQUIRE(yes.asBool());
        REQUIRE(yes.asInt() == 1);
        REQUIRE(yes.asFloat() == 1.0);
        REQUIRE(yes.toString() == "True");
        REQUIRE(no.isBool());
        REQUIRE_FALSE(no.asBool());
        REQUIRE(no.asInt() == 0);
        REQUIRE(no.asFloat() == 0.0);
        REQUIRE(no.toString() == "False");
    }

    SECTION("integer")
    {
        const Value value{std::int64_t{42}};

        REQUIRE(value.type() == ValueType::Int);
        REQUIRE(value.isInt());
        REQUIRE(value.isNumber());
        REQUIRE(value.asInt() == 42);
        REQUIRE(value.asFloat() == 42.0);
        REQUIRE(value.toString() == "42");
    }

    SECTION("float")
    {
        const Value value{3.5};

        REQUIRE(value.type() == ValueType::Float);
        REQUIRE(value.isFloat());
        REQUIRE(value.isNumber());
        REQUIRE(value.asFloat() == 3.5);
        REQUIRE(value.asInt() == 3);
        REQUIRE(value.toString() == "3.5");
    }

    SECTION("string")
    {
        const Value value{"Euler"};

        REQUIRE(value.type() == ValueType::String);
        REQUIRE(value.isString());
        REQUIRE(value.asString() == "Euler");
        REQUIRE(value.toString() == "Euler");
        REQUIRE(value.length() == 5);
    }
}

TEST_CASE("Value represents list and map runtime containers", "[token][jinja][vm][value][usage][container]")
{
    SECTION("list")
    {
        const Value value{ValueList{Value{"one"}, Value{"two"}, Value{"three"}}};

        REQUIRE(value.type() == ValueType::List);
        REQUIRE(value.isList());
        REQUIRE(value.length() == 3);
        REQUIRE(value.getItem(Value{0}).toString() == "one");
        REQUIRE(value.getItem(Value{2}).toString() == "three");
    }

    SECTION("map")
    {
        ValueMap map;
        map["name"] = Value{"Euler"};
        map["occupation"] = Value{"drunk cousin"};

        const Value value{std::move(map)};

        REQUIRE(value.type() == ValueType::Map);
        REQUIRE(value.isMap());
        REQUIRE(value.length() == 2);
        REQUIRE(value.getItem(Value{"name"}).toString() == "Euler");
        REQUIRE(value.getItem(Value{"occupation"}).toString() == "drunk cousin");
    }
}

TEST_CASE("Value supports negative list and string indexing", "[token][jinja][vm][value][usage][index]")
{
    const Value list{ValueList{Value{"A"}, Value{"B"}, Value{"C"}}};

    REQUIRE(list.getItem(Value{-1}).toString() == "C");
    REQUIRE(list.getItem(Value{-2}).toString() == "B");

    const Value text{"Euler"};

    REQUIRE(text.getItem(Value{-1}).toString() == "r");
    REQUIRE(text.getItem(Value{0}).toString() == "E");
}

TEST_CASE("Value setItem updates map and list entries", "[token][jinja][vm][value][usage][mutation]")
{
    SECTION("map")
    {
        Value value{ValueMap{}};

        value.setItem(Value{"answer"}, Value{42});

        REQUIRE(value.getItem(Value{"answer"}).asInt() == 42);
    }

    SECTION("list")
    {
        Value value{ValueList{Value{"A"}, Value{"B"}, Value{"C"}}};

        value.setItem(Value{-1}, Value{"Z"});

        REQUIRE(value.getItem(Value{2}).toString() == "Z");
    }
}

TEST_CASE("Value truthiness follows runtime type", "[token][jinja][vm][value][usage][truthy]")
{
    REQUIRE_FALSE(Value{}.isTruthy());
    REQUIRE_FALSE(Value{false}.isTruthy());
    REQUIRE(Value{true}.isTruthy());
    REQUIRE_FALSE(Value{0}.isTruthy());
    REQUIRE(Value{1}.isTruthy());
    REQUIRE_FALSE(Value{0.0}.isTruthy());
    REQUIRE(Value{0.5}.isTruthy());
    REQUIRE_FALSE(Value{""}.isTruthy());
    REQUIRE(Value{"x"}.isTruthy());
    REQUIRE_FALSE(Value{ValueList{}}.isTruthy());
    REQUIRE(Value{ValueList{Value{1}}}.isTruthy());
    REQUIRE_FALSE(Value{ValueMap{}}.isTruthy());

    ValueMap map;
    map["x"] = Value{1};

    REQUIRE(Value{std::move(map)}.isTruthy());
}

TEST_CASE("Value numeric equality crosses integer and float types", "[token][jinja][vm][value][usage][comparison]")
{
    const Value integer{42};
    const Value floating{42.0};

    REQUIRE(integer == floating);
    REQUIRE_FALSE(integer != floating);
    REQUIRE(Value{1} < Value{2});
    REQUIRE(Value{2} > Value{1});
    REQUIRE(Value{"abc"} < Value{"xyz"});
}

TEST_CASE("JinjaVM renders text and expressions", "[token][jinja][vm][usage]")
{
    static constexpr std::string_view Source = "Hello {{ name }}!";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    REQUIRE(ast != nullptr);

    ValueMap context;
    context["name"] = Value{"Joseph"};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "Hello Joseph!");
}

TEST_CASE("JinjaVM renders runtime scalar values", "[token][jinja][vm][usage][value]")
{
    static constexpr std::string_view Source = "{{ truth }}|{{ lie }}|{{ integer }}|{{ floating }}|{{ nothing }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["truth"] = Value{true};
    context["lie"] = Value{false};
    context["integer"] = Value{42};
    context["floating"] = Value{3.5};
    context["nothing"] = Value{};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "True|False|42|3.5|");
}

TEST_CASE("JinjaVM evaluates arithmetic expressions", "[token][jinja][vm][usage][operator]")
{
    static constexpr std::string_view Source =
        "{{ 2 + 3 }}|"
        "{{ 10 - 4 }}|"
        "{{ 6 * 7 }}|"
        "{{ 7 / 2 }}|"
        "{{ 7 // 2 }}|"
        "{{ 7 % 4 }}|"
        "{{ 2 ** 4 }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "5|6|42|3.5|3|3|16");
}

TEST_CASE("JinjaVM observes arithmetic precedence", "[token][jinja][vm][usage][operator][precedence]")
{
    static constexpr std::string_view Source =
        "{{ 1 + 2 * 3 }}|"
        "{{ (1 + 2) * 3 }}|"
        "{{ 2 * 3 ** 2 }}|"
        "{{ 2 ** 3 ** 2 }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "7|9|18|512");
}

TEST_CASE("JinjaVM evaluates unary operators", "[token][jinja][vm][usage][operator][unary]")
{
    static constexpr std::string_view Source = "{{ -5 }}|" "{{ +5 }}|" "{{ not false }}|" "{{ not true }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "-5|5|True|False");
}

TEST_CASE("JinjaVM concatenates values", "[token][jinja][vm][usage][operator][concat]")
{
    static constexpr std::string_view Source = "{{ 'Euler' ~ ' ' ~ 42 }}|" "{{ 'Euler' + ' cousin' }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "Euler 42|Euler cousin");
}

TEST_CASE("JinjaVM evaluates comparisons", "[token][jinja][vm][usage][comparison]")
{
    static constexpr std::string_view Source =
        "{{ 2 == 2 }}|"
        "{{ 2 != 3 }}|"
        "{{ 2 < 3 }}|"
        "{{ 2 <= 2 }}|"
        "{{ 3 > 2 }}|"
        "{{ 3 >= 3 }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "True|True|True|True|True|True");
}

TEST_CASE("JinjaVM evaluates membership", "[token][jinja][vm][usage][membership]")
{
    static constexpr std::string_view Source =
        "{{ 2 in numbers }}|"
        "{{ 9 not in numbers }}|"
        "{{ 'ell' in text }}|"
        "{{ 'name' in object }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap object;
    object["name"] = Value{"Euler"};

    ValueMap context;
    context["numbers"] = Value{ValueList{Value{1}, Value{2}, Value{3}}};
    context["text"] = Value{"hello"};
    context["object"] = Value{std::move(object)};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "True|True|True|True");
}

TEST_CASE("JinjaVM evaluates Jinja is tests", "[token][jinja][vm][usage][test]")
{
    static constexpr std::string_view Source =
        "{{ missing is none }}|"
        "{{ value is defined }}|"
        "{{ even is even }}|"
        "{{ odd is odd }}|"
        "{{ text is string }}|"
        "{{ number is number }}|"
        "{{ truth is true }}|"
        "{{ lie is false }}|"
        "{{ text is not number }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["value"] = Value{"defined"};
    context["even"] = Value{4};
    context["odd"] = Value{5};
    context["text"] = Value{"Euler"};
    context["number"] = Value{42.0};
    context["truth"] = Value{true};
    context["lie"] = Value{false};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "True|True|True|True|True|True|True|True|True");
}

TEST_CASE("JinjaVM evaluates conditional expression", "[token][jinja][vm][usage][conditional]")
{
    static constexpr std::string_view Source = "{{ 'YES' if enabled else 'NO' }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    ValueMap enabled;
    enabled["enabled"] = Value{true};

    REQUIRE(vm.execute(*ast, enabled) == "YES");

    ValueMap disabled;
    disabled["enabled"] = Value{false};

    REQUIRE(vm.execute(*ast, disabled) == "NO");
}

TEST_CASE("JinjaVM executes if elif else branches", "[token][jinja][vm][usage][if]")
{
    static constexpr std::string_view Source =
        "{% if value == 1 %}"
        "ONE"
        "{% elif value == 2 %}"
        "TWO"
        "{% else %}"
        "OTHER"
        "{% endif %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    ValueMap one;
    one["value"] = Value{1};

    REQUIRE(vm.execute(*ast, one) == "ONE");

    ValueMap two;
    two["value"] = Value{2};

    REQUIRE(vm.execute(*ast, two) == "TWO");

    ValueMap other;
    other["value"] = Value{99};

    REQUIRE(vm.execute(*ast, other) == "OTHER");
}

TEST_CASE("JinjaVM executes for loop over list", "[token][jinja][vm][usage][for]")
{
    static constexpr std::string_view Source = "{% for item in items %}" "{{ item }}" "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["items"] = Value{ValueList{Value{"A"}, Value{"B"}, Value{"C"}}};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "ABC");
}

TEST_CASE("JinjaVM exposes loop index and boundary variables", "[token][jinja][vm][usage][for][loop]")
{
    static constexpr std::string_view Source =
        "{% for item in items %}"
        "{{ loop.index0 }}:"
        "{{ loop.index }}:"
        "{{ loop.first }}:"
        "{{ loop.last }}:"
        "{{ loop.length }}:"
        "{{ item }};"
        "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["items"] = Value{ValueList{Value{"A"}, Value{"B"}, Value{"C"}}};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) ==
            "0:1:True:False:3:A;"
            "1:2:False:False:3:B;"
            "2:3:False:True:3:C;");
}

TEST_CASE("JinjaVM exposes loop previous and next items", "[token][jinja][vm][usage][for][loop]")
{
    static constexpr std::string_view Source =
        "{% for item in items %}"
        "[{{ loop.previtem }}|{{ item }}|{{ loop.nextitem }}]"
        "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["items"] = Value{ValueList{Value{"A"}, Value{"B"}, Value{"C"}}};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "[|A|B]" "[A|B|C]" "[B|C|]");
}

TEST_CASE("JinjaVM executes for else on empty iterable", "[token][jinja][vm][usage][for][else]")
{
    static constexpr std::string_view Source = "{% for item in items %}" "{{ item }}" "{% else %}" "EMPTY" "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["items"] = Value{ValueList{}};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "EMPTY");
}

TEST_CASE("JinjaVM unpacks list values into multiple loop targets", "[token][jinja][vm][usage][for][unpack]")
{
    static constexpr std::string_view Source = "{% for key, value in pairs %}" "{{ key }}={{ value }};" "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueList pairs;
    pairs.emplace_back(ValueList{Value{"A"}, Value{1}});
    pairs.emplace_back(ValueList{Value{"B"}, Value{2}});

    ValueMap context;
    context["pairs"] = Value{std::move(pairs)};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "A=1;B=2;");
}

TEST_CASE("JinjaVM set statement creates runtime variable", "[token][jinja][vm][usage][set]")
{
    static constexpr std::string_view Source = "{% set answer = 40 + 2 %}" "{{ answer }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "42");
}

TEST_CASE("JinjaVM resolves map members", "[token][jinja][vm][usage][member]")
{
    static constexpr std::string_view Source = "{{ message.role }}:" "{{ message.content }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap message;
    message["role"] = Value{"user"};
    message["content"] = Value{"Hello"};

    ValueMap context;
    context["message"] = Value{std::move(message)};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "user:Hello");
}

TEST_CASE("JinjaVM evaluates list and map subscripts", "[token][jinja][vm][usage][subscript]")
{
    static constexpr std::string_view Source = "{{ items[1] }}|" "{{ items[-1] }}|" "{{ object['name'] }}|" "{{ text[0] }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap object;
    object["name"] = Value{"Euler"};

    ValueMap context;
    context["items"] = Value{ValueList{Value{"A"}, Value{"B"}, Value{"C"}}};
    context["object"] = Value{std::move(object)};
    context["text"] = Value{"JOB"};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "B|C|Euler|J");
}

TEST_CASE("JinjaVM evaluates list and dictionary literals", "[token][jinja][vm][usage][literal][container]")
{
    static constexpr std::string_view Source = "{{ [1, 2, 3] | length }}|" "{{ {'a': 1, 'b': 2} | length }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "3|2");
}

TEST_CASE("JinjaVM applies built in string filters", "[token][jinja][vm][usage][filter]")
{
    static constexpr std::string_view Source = "{{ text | trim }}|" "{{ text | trim | lower }}|" "{{ text | trim | upper }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["text"] = Value{"  EuLeR  "};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "EuLeR|euler|EULER");
}

TEST_CASE("JinjaVM applies length and count filters", "[token][jinja][vm][usage][filter][length]")
{
    static constexpr std::string_view Source = "{{ text | length }}|" "{{ items | count }}|" "{{ object | length }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap object;
    object["a"] = Value{1};
    object["b"] = Value{2};

    ValueMap context;
    context["text"] = Value{"Euler"};
    context["items"] = Value{ValueList{Value{1}, Value{2}, Value{3}}};
    context["object"] = Value{std::move(object)};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "5|3|2");
}

TEST_CASE("JinjaVM applies first and last filters", "[token][jinja][vm][usage][filter]")
{
    static constexpr std::string_view Source = "{{ items | first }}|" "{{ items | last }}|" "{{ text | first }}|" "{{ text | last }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["items"] = Value{ValueList{Value{"A"}, Value{"B"}, Value{"C"}}};
    context["text"] = Value{"Euler"};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "A|C|E|r");
}

TEST_CASE("JinjaVM applies default filter", "[token][jinja][vm][usage][filter][default]")
{
    static constexpr std::string_view Source =
        "{{ missing | default('fallback') }}|"
        "{{ empty | default('fallback') }}|"
        "{{ value | default('fallback') }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["empty"] = Value{""};
    context["value"] = Value{"Euler"};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "fallback|fallback|Euler");
}

TEST_CASE("JinjaVM joins lists", "[token][jinja][vm][usage][filter][join]")
{
    static constexpr std::string_view Source = "{{ items | join(',') }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["items"] = Value{ValueList{Value{"A"}, Value{"B"}, Value{"C"}}};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "A,B,C");
}

TEST_CASE("JinjaVM range function creates ascending sequence", "[token][jinja][vm][usage][function][range]")
{
    static constexpr std::string_view Source = "{% for value in range(4) %}" "{{ value }}" "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "0123");
}

TEST_CASE("JinjaVM range function supports start stop and step", "[token][jinja][vm][usage][function][range]")
{
    static constexpr std::string_view Source = "{% for value in range(2, 10, 2) %}" "{{ value }}," "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "2,4,6,8,");
}

TEST_CASE("JinjaVM range function supports descending sequence", "[token][jinja][vm][usage][function][range]")
{
    static constexpr std::string_view Source = "{% for value in range(5, 0, -2) %}" "{{ value }}," "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "5,3,1,");
}

TEST_CASE("JinjaVM iterates map items into key value loop targets", "[token][jinja][vm][usage][map][items]")
{
    static constexpr std::string_view Source = "{% for key, value in object.items() %}" "{{ key }}={{ value }};" "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap object;
    object["alpha"] = Value{1};
    object["beta"] = Value{2};

    ValueMap context;
    context["object"] = Value{std::move(object)};

    JinjaVM vm;

    const std::string result = vm.execute(*ast, context);

    //
    // ValueMap is unordered. Do not make iteration order part of
    // the template contract.
    //
    REQUIRE(result.size() == std::string{"alpha=1;beta=2;"}.size());
    REQUIRE(result.find("alpha=1;") != std::string::npos);
    REQUIRE(result.find("beta=2;") != std::string::npos);
}

TEST_CASE("JinjaVM direct map iteration yields keys", "[token][jinja][vm][usage][map][for]")
{
    static constexpr std::string_view Source = "{% for key in object %}" "{{ key }};" "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap object;
    object["alpha"] = Value{1};
    object["beta"] = Value{2};

    ValueMap context;
    context["object"] = Value{std::move(object)};

    JinjaVM vm;

    const std::string result = vm.execute(*ast, context);

    REQUIRE(result.find("alpha;") != std::string::npos);
    REQUIRE(result.find("beta;") != std::string::npos);
}

TEST_CASE("JinjaVM registerFilter adds custom runtime filter", "[token][jinja][vm][usage][filter][custom]")
{
    static constexpr std::string_view Source = "{{ value | surround('[', ']') }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    vm.registerFilter("surround", [](const Value &target, const std::vector<Value> &args, const ValueMap &) -> Value {
        const std::string left = args.size() > 0 ? args[0].toString() : "";
        const std::string right = args.size() > 1 ? args[1].toString() : "";
        return Value{left + target.toString() + right};
    });

    ValueMap context;
    context["value"] = Value{"Euler"};

    REQUIRE(vm.execute(*ast, context) == "[Euler]");
}

TEST_CASE("JinjaVM registerFunction adds custom runtime function", "[token][jinja][vm][usage][function][custom]")
{
    static constexpr std::string_view Source = "{{ add(20, 22) }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    vm.registerFunction("add", [](const std::vector<Value> &args, const ValueMap &) -> Value {
        if (args.size() < 2)
            return Value{};
        return Value{args[0].asInt() + args[1].asInt()};
    });

    REQUIRE(vm.execute(*ast, ValueMap{}) == "42");
}

TEST_CASE("JinjaVM passes keyword arguments to custom function", "[token][jinja][vm][usage][function][keyword]")
{
    static constexpr std::string_view Source = "{{ describe(name='Euler', quality='drunk') }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    vm.registerFunction("describe", [](const std::vector<Value> &, const ValueMap &kwargs) -> Value {
        const auto name = kwargs.find("name");
        const auto quality = kwargs.find("quality");

        if (name == kwargs.end() || quality == kwargs.end())
            return Value{};

        return Value{name->second.toString() + ":" + quality->second.toString()};
    });

    REQUIRE(vm.execute(*ast, ValueMap{}) == "Euler:drunk");
}

TEST_CASE("JinjaVM evaluate returns expression value without rendering template", "[token][jinja][vm][usage][evaluate]")
{
    static constexpr std::string_view Source = "{{ answer + 2 }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    REQUIRE(ast != nullptr);
    REQUIRE(ast->statements.size() == 1);

    const auto *output = static_cast<const job::token::OutputNode *>(ast->statements[0].get());

    REQUIRE(output->expression != nullptr);

    ValueMap context;
    context["answer"] = Value{40};

    JinjaVM vm;

    const Value result = vm.evaluate(*output->expression, context);

    REQUIRE(result.isInt());
    REQUIRE(result.asInt() == 42);
}

TEST_CASE("JinjaVM renders realistic ChatML template", "[token][jinja][vm][usage][chat]")
{
    static constexpr std::string_view Source =
        "{% for message in messages %}"
        "<|im_start|>{{ message.role }}\n"
        "{{ message.content }}<|im_end|>\n"
        "{% endfor %}"
        "{% if add_generation_prompt %}"
        "<|im_start|>assistant\n"
        "{% endif %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueList messages;

    {
        ValueMap message;
        message["role"] = Value{"system"};
        message["content"] = Value{"You are helpful."};
        messages.emplace_back(std::move(message));
    }

    {
        ValueMap message;
        message["role"] = Value{"user"};
        message["content"] = Value{"Hello!"};
        messages.emplace_back(std::move(message));
    }

    ValueMap context;
    context["messages"] = Value{std::move(messages)};
    context["add_generation_prompt"] = Value{true};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) ==
            "<|im_start|>system\n"
            "You are helpful.<|im_end|>\n"
            "<|im_start|>user\n"
            "Hello!<|im_end|>\n"
            "<|im_start|>assistant\n");
}

//
// Block 2: edge cases / invariants
//

TEST_CASE("JinjaVM undefined variable renders empty string", "[token][jinja][vm][edge][undefined]")
{
    static constexpr std::string_view Source = "before[{{ missing }}]after";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "before[]after");
}

TEST_CASE("JinjaVM missing member and subscript render empty string", "[token][jinja][vm][edge][undefined]")
{
    static constexpr std::string_view Source = "[{{ object.missing }}]" "[{{ items[99] }}]" "[{{ text[99] }}]";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["object"] = Value{ValueMap{}};
    context["items"] = Value{ValueList{Value{"A"}}};
    context["text"] = Value{"A"};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "[][][]");
}

TEST_CASE("JinjaVM unknown filter leaves target unchanged", "[token][jinja][vm][edge][filter]")
{
    static constexpr std::string_view Source = "{{ value | this_filter_drank_the_manual }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["value"] = Value{"Euler"};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "Euler");
}

TEST_CASE("JinjaVM unknown function evaluates to none", "[token][jinja][vm][edge][function]")
{
    static constexpr std::string_view Source = "before[{{ definitely_not_a_function() }}]after";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "before[]after");
}

TEST_CASE("JinjaVM unknown member function evaluates to none", "[token][jinja][vm][edge][function][member]")
{
    static constexpr std::string_view Source = "[{{ object.wander_off() }}]";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["object"] = Value{ValueMap{}};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "[]");
}

TEST_CASE("JinjaVM and returns operands with short circuit semantics", "[token][jinja][vm][edge][short-circuit]")
{
    static constexpr std::string_view Source = "{{ empty and missing.field }}|" "{{ value and second }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["empty"] = Value{""};
    context["value"] = Value{"first"};
    context["second"] = Value{"second"};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "|second");
}

TEST_CASE("JinjaVM or returns operands with short circuit semantics", "[token][jinja][vm][edge][short-circuit]")
{
    static constexpr std::string_view Source = "{{ value or missing.field }}|" "{{ empty or fallback }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["value"] = Value{"first"};
    context["empty"] = Value{""};
    context["fallback"] = Value{"second"};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "first|second");
}

TEST_CASE("JinjaVM division and modulo by zero return zero", "[token][jinja][vm][edge][arithmetic]")
{
    static constexpr std::string_view Source = "{{ 10 / 0 }}|" "{{ 10 // 0 }}|" "{{ 10 % 0 }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "0|0|0");
}

TEST_CASE("JinjaVM empty first and last filters produce none", "[token][jinja][vm][edge][filter]")
{
    static constexpr std::string_view Source = "[{{ items | first }}]" "[{{ items | last }}]" "[{{ text | first }}]" "[{{ text | last }}]";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["items"] = Value{ValueList{}};
    context["text"] = Value{""};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "[][][][]");
}

TEST_CASE("JinjaVM join on non list passes target through", "[token][jinja][vm][edge][filter]")
{
    static constexpr std::string_view Source = "{{ value | join(',') }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["value"] = Value{"Euler"};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "Euler");
}

TEST_CASE("JinjaVM range with zero step currently falls back to one", "[token][jinja][vm][edge][function][range]")
{
    static constexpr std::string_view Source = "{% for value in range(0, 3, 0) %}" "{{ value }}" "{% endfor %}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, ValueMap{}) == "012");
}

TEST_CASE("JinjaVM set inside loop does not leak loop scope", "[token][jinja][vm][edge][scope]")
{
    static constexpr std::string_view Source =
        "{% for item in items %}"
        "{% set temporary = item %}"
        "{{ temporary }}"
        "{% endfor %}"
        "[{{ temporary }}]";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["items"] = Value{ValueList{Value{"A"}, Value{"B"}}};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "AB[]");
}

TEST_CASE("JinjaVM loop variable does not leak after loop", "[token][jinja][vm][edge][scope]")
{
    static constexpr std::string_view Source = "{% for item in items %}" "{{ item }}" "{% endfor %}" "[{{ item }}]";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    ValueMap context;
    context["items"] = Value{ValueList{Value{"A"}, Value{"B"}}};

    JinjaVM vm;

    REQUIRE(vm.execute(*ast, context) == "AB[]");
}

TEST_CASE("JinjaVM execute clears output between runs", "[token][jinja][vm][edge][reuse]")
{
    static constexpr std::string_view Source = "{{ value }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    JinjaVM vm;

    ValueMap first;
    first["value"] = Value{"FIRST"};

    REQUIRE(vm.execute(*ast, first) == "FIRST");

    ValueMap second;
    second["value"] = Value{"SECOND"};

    REQUIRE(vm.execute(*ast, second) == "SECOND");
}

TEST_CASE("JinjaVM evaluate clears scope between calls", "[token][jinja][vm][edge][reuse]")
{
    static constexpr std::string_view Source = "{{ value }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    const auto *output = static_cast<const job::token::OutputNode *>(ast->statements[0].get());

    JinjaVM vm;

    ValueMap first;
    first["value"] = Value{"FIRST"};

    REQUIRE(vm.evaluate(*output->expression, first).toString() == "FIRST");
    REQUIRE(vm.evaluate(*output->expression, ValueMap{}).isNone());
}

TEST_CASE("JinjaVM Value out of range indexing returns none", "[token][jinja][vm][value][edge][index]")
{
    const Value list{ValueList{Value{"A"}}};

    REQUIRE(list.getItem(Value{99}).isNone());
    REQUIRE(list.getItem(Value{-99}).isNone());

    const Value text{"A"};

    REQUIRE(text.getItem(Value{99}).isNone());
}

TEST_CASE("JinjaVM Value setItem ignores unsupported targets and indexes", "[token][jinja][vm][value][edge][mutation]")
{
    Value scalar{42};

    scalar.setItem(Value{0}, Value{99});

    REQUIRE(scalar.asInt() == 42);

    Value list{ValueList{Value{"A"}}};

    list.setItem(Value{99}, Value{"B"});

    REQUIRE(list.getItem(Value{0}).toString() == "A");
}

//
// Block 3: benchmarks
//

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Benchmark JinjaVM evaluation performance", "[token][jinja][vm][benchmark]")
{
    static constexpr std::string_view Source =
        "{% for message in messages %}"
        "<|im_start|>{{ message.role }}\n"
        "{{ message.content }}<|im_end|>\n"
        "{% endfor %}"
        "{% if add_generation_prompt %}"
        "<|im_start|>assistant\n"
        "{% endif %}";

    JinjaLexer lexer{Source};

    //
    // Keep lexer storage alive through parsing.
    //
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();

    JinjaParser parser{tokens};

    //
    // Compile once. This benchmark is VM evaluation only.
    //
    const std::unique_ptr<BodyNode> ast = parser.parse();

    REQUIRE(ast != nullptr);

    ValueList messages;

    for (std::int64_t i = 0; i < 20; ++i) {
        ValueMap message;
        message["role"] = Value{i % 2 == 0 ? "user" : "assistant"};
        message["content"] = Value{"Benchmarking message contents for iteration " + std::to_string(i)};
        messages.emplace_back(std::move(message));
    }

    ValueMap context;
    context["messages"] = Value{std::move(messages)};
    context["add_generation_prompt"] = Value{true};

    JinjaVM vm;

    BENCHMARK("Evaluate 20-message ChatML prompt in VM")
    {
        return vm.execute(*ast, context);
    };
}

TEST_CASE("Benchmark JinjaVM expression evaluation", "[token][jinja][vm][benchmark][expression]")
{
    static constexpr std::string_view Source = "{{ ((a + b) * c) ** 2 }}";

    JinjaLexer lexer{Source};
    const std::vector<JinjaToken> tokens = lexer.tokenizeAll();
    JinjaParser parser{tokens};
    const std::unique_ptr<BodyNode> ast = parser.parse();

    REQUIRE(ast != nullptr);
    REQUIRE(ast->statements.size() == 1);

    const auto *output = static_cast<const job::token::OutputNode *>(ast->statements[0].get());

    REQUIRE(output->expression != nullptr);

    ValueMap context;
    context["a"] = Value{10};
    context["b"] = Value{20};
    context["c"] = Value{3};

    JinjaVM vm;

    BENCHMARK("Evaluate arithmetic expression in VM")
    {
        return vm.evaluate(*output->expression, context);
    };
}

#endif