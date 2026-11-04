#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <simdjson.h>

#include <clp/StringReader.hpp>
#include <clp_s/JsonFileIterator.hpp>

namespace {
auto read_values(std::string const& input, bool sanitize, size_t buffer_size)
        -> std::vector<std::string> {
    clp::StringReader reader;
    reader.open(input);
    clp_s::JsonFileIterator iterator{reader, 4096, sanitize, {}, buffer_size};
    REQUIRE(simdjson::SUCCESS == iterator.get_error());
    simdjson::ondemand::document_stream::iterator document;
    std::vector<std::string> values;
    while (iterator.get_json(document)) {
        std::string_view value;
        simdjson::ondemand::object object;
        auto reference = *document;
        REQUIRE(simdjson::SUCCESS == reference.get_object().get(object));
        REQUIRE(simdjson::SUCCESS == object["value"].get_string(true).get(value));
        values.emplace_back(value);
        REQUIRE(values.size() <= 10);
    }
    REQUIRE(simdjson::SUCCESS == iterator.get_error());
    REQUIRE(0 == iterator.truncated_bytes());
    return values;
}
}  // namespace

TEST_CASE("clp-s-json-sanitization-streaming", "[clp-s][JsonFileIterator]") {
    size_t const buffer_size = GENERATE(32, 64, 128, 1024);
    CAPTURE(buffer_size);
    SECTION("Control characters and invalid UTF-8 do not duplicate earlier records") {
        std::string const input{"{\"value\":\"first\"}\n{\"value\":\"bad\ncontrol\"}\n"
                                "{\"value\":\"invalid\xff\"}\n{\"value\":\"last\"}\n"};
        auto const values{read_values(input, true, buffer_size)};
        REQUIRE(
                values
                == std::vector<std::string>{"first", "bad\ncontrol", "invalid\xef\xbf\xbd", "last"}
        );
    }
    SECTION("Valid multibyte characters spanning buffers are preserved") {
        for (size_t padding{0}; padding < 8; ++padding) {
            CAPTURE(padding);
            std::string const text{std::string(15 + padding, 'x') + "\xf0\x9f\x98\x80"};
            std::string const input{"{\"value\":\"" + text + "\"}\n{\"value\":\"last\"}\n"};
            REQUIRE(read_values(input, true, buffer_size)
                    == std::vector<std::string>{text, "last"});
            REQUIRE(read_values(input, false, buffer_size)
                    == std::vector<std::string>{text, "last"});
        }
    }
}

TEST_CASE("clp-s-json-sanitization-disabled", "[clp-s][JsonFileIterator]") {
    for (std::string const input : {"{\"value\":\"bad\ncontrol\"}\n", "{\"value\":\"bad\xff\"}\n"})
    {
        clp::StringReader reader;
        reader.open(input);
        clp_s::JsonFileIterator iterator{reader, 4096, false};
        simdjson::ondemand::document_stream::iterator document;
        REQUIRE_FALSE(iterator.get_json(document));
        REQUIRE(simdjson::SUCCESS != iterator.get_error());
    }
}
