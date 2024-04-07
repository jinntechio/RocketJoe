#pragma once

#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace components::serialization::traits {

    struct array_tag {};
    struct list_tag {};
    struct object_tag {};
    struct string_tag {};
    struct pair_tag {};
    struct tuple_tag {};
    struct variant_tag {};
    struct char_tag {};
    struct boolean_tag {};
    struct signed_number_tag {};
    struct unsigned_number_tag {};
    struct null_tag {};

    struct class_tag {};
    struct struct_tag {};
    struct contaner_tag {};

    template<typename>
    struct is_tuple : std::false_type {};

    template<typename... T>
    struct is_tuple<std::tuple<T...>> : std::true_type {};

    template<typename T>
    constexpr bool is_tuple_v = is_tuple<T>::value;

    template<class C, typename = void>
    struct category_trait_impl : std::false_type {
        using category = null_tag;
    };

    template<class C, class Allocator>
    struct category_trait_impl<std::vector<C, Allocator>, void> : std::true_type {
        using category = array_tag;
    };

    template<class Key, class Tp, class Compare, class Allocator>
    struct category_trait_impl<std::map<Key, Tp, Compare, Allocator>, void> : std::true_type {
        using category = object_tag;
    };

    template<class C, class Traits, class Alloc>
    struct category_trait_impl<std::basic_string<C, Traits, Alloc>, void> : std::true_type {
        using category = string_tag;
    };

    template<class C, class Traits>
    struct category_trait_impl<std::basic_string_view<C, Traits>, void> : std::true_type {
        using category = string_tag;
    };

    template<class T>
    struct category_trait_impl<T,
                               std::void_t<decltype(std::declval<T>().index()),
                                           decltype(std::variant_size<T>::value),
                                           typename std::variant_alternative<0, T>::type,
                                           decltype(std::get<std::variant_alternative_t<0, T>>(std::declval<T>()))>>
        : std::is_same<size_t, std::decay_t<decltype(std::declval<T>().index())>> {
        using category = variant_tag;
    };

    template<class T1, class T2>
    struct category_trait_impl<std::pair<T1, T2>, void> : std::true_type {
        using category = pair_tag;
    };

    template<class T, size_t... I>
    struct category_trait_impl<std::tuple<T, std::index_sequence<I...>>, void> : std::true_type {
        using category = tuple_tag;
    };

    template<>
    struct category_trait_impl<bool, void> {
        using category = boolean_tag;
    };

    template<>
    struct category_trait_impl<char, void> {
        using category = char_tag;
    };

    template<>
    struct category_trait_impl<int8_t, void> {
        using category = signed_number_tag;
    };

    template<>
    struct category_trait_impl<int16_t, void> {
        using category = signed_number_tag;
    };

    template<>
    struct category_trait_impl<int32_t, void> {
        using category = signed_number_tag;
    };

    template<>
    struct category_trait_impl<int64_t, void> {
        using category = signed_number_tag;
    };

    template<>
    struct category_trait_impl<uint8_t, void> {
        using category = unsigned_number_tag;
    };

    template<>
    struct category_trait_impl<uint16_t, void> {
        using category = unsigned_number_tag;
    };

    template<>
    struct category_trait_impl<uint32_t, void> {
        using category = unsigned_number_tag;
    };

    template<>
    struct category_trait_impl<uint64_t, void> {
        using category = unsigned_number_tag;
    };

    template<class C>
    struct serialization_trait {
        using category = typename category_trait_impl<C>::category;
    };

} // namespace components::serialization::traits