#include <catch2/catch.hpp>

#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>

#include <components/serialization/serialization.hpp>

using components::serialization::serialize;
using components::serialization::serialize_array;
using components::serialization::context::json_context;

TEST_CASE("base tuple") {
    json_context json;
    std::vector<int64_t> vector{1, 2, 3};
    std::map<int64_t, int64_t> map = {{1, 2}, {3, 4}};
    std::string str("42");
    std::string_view str1("42");
    std::uint64_t number = 42;
    auto tpl = std::tie(number, str, str1, vector, map);
    serialize_array(json, std::tuple_size_v<decltype(tpl)>);
    serialize(json, tpl);
    std::cerr << json.data() << std::endl;
    std::exit(1);
}