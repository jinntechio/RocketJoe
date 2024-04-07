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

class no_dto_t {
public:
    no_dto_t() = default;

    auto access_as_tuple() noexcept {
        return std::tie(example_0_, example_1_, example_2_, example_3_); // NOLINT
    }

private:
    std::uint64_t example_0_{42};
    std::string example_1_{"42"};
    std::vector<uint64_t> example_2_{1, 2, 3};
    std::map<std::string_view, uint64_t> example_3_{{"1", 2}, {"3", 4}};
};

TEST_CASE("base class") {
    json_context json;
    no_dto_t no_dto;
    serialize_array(json, std::tuple_size_v<decltype(no_dto.access_as_tuple())>);
    serialize(json, no_dto);
    std::cout << json.data() << std::endl;
    std::exit(1);
}