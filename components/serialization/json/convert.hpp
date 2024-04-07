#pragma once

#include <string>
#include <string_view>

namespace components::serialization::detail {

    template<class T>
    std::string convert(T& t) {
        return std::to_string(t);
    }

    std::string convert(const std::string& t);
    std::string convert(std::string_view t);
} // namespace components::serialization::detail