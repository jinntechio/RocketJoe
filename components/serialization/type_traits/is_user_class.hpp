#pragma once

#include <type_traits>

namespace components::serialization::traits {

    template<typename, typename = std::void_t<>>
    struct has_access_as_tuple : public std::false_type {};

    template<typename T>
    struct has_access_as_tuple<T, std::void_t<decltype(std::declval<T&>().access_as_tuple())>>
        : public std::true_type {};

    template<typename T>
    struct is_user_class {
        static constexpr bool value = has_access_as_tuple<T>::value;
    };

    template<typename T>
    constexpr inline bool is_user_class_v = is_user_class<T>::value;

} // namespace components::serialization::traits
