#pragma once

#include <type_traits>

namespace components::serialization::traits {

// has_value_type
template<typename, typename = std::void_t<>>
struct has_value_type : public std::false_type {};

template<typename T>
struct has_value_type<T, std::void_t<typename T::value_type>> : public std::true_type {};

// has_key_type
template<typename, typename = std::void_t<>>
struct has_key_type : public std::false_type {};

template<typename T>
struct has_key_type<T, std::void_t<typename T::key_type>> : public std::true_type {};


// has_mapped_type
template<typename, typename = std::void_t<>>
struct has_mapped_type : public std::false_type {};

template<typename T>
struct has_mapped_type<T, std::void_t<typename T::mapped_type>> : public std::true_type {};

// has_iterator_type
template<typename, typename = std::void_t<>>
struct has_iterator_type : public std::false_type {};

template<typename T>
struct has_iterator_type<T, std::void_t<typename T::iterator>> : public std::true_type {};


// has_const_iterator_type
template<typename, typename = std::void_t<>>
struct has_const_iterator_type : public std::false_type {};

template<typename T>
struct has_const_iterator_type<T, std::void_t<typename T::const_iterator>> : public std::true_type {};


// has_begin
template<typename, typename = std::void_t<>>
struct has_begin : public std::false_type {};

template<typename T>
struct has_begin<T, std::void_t<decltype(std::declval<const T&>().begin())>> : public std::true_type {};


// has_end
template<typename, typename = std::void_t<>>
struct has_end : public std::false_type {};

template<typename T>
struct has_end<T, std::void_t<decltype(std::declval<const T&>().end())>> : public std::true_type {};


// has_emplace
template<typename, typename = std::void_t<>>
struct has_emplace : public std::false_type {};

template<typename T>
struct has_emplace<T, std::void_t<decltype(std::declval<T&>().emplace(std::declval<typename T::value_type>()))>>
    : public std::true_type {};

// has_emplace_back
template<typename, typename = std::void_t<>>
struct has_emplace_back : public std::false_type {};

template<typename T>
struct has_emplace_back<T, std::void_t<decltype(std::declval<T&>().emplace_back(std::declval<typename T::value_type>()))>>
    : public std::true_type {};

// has_emplace_after
template<typename, typename = std::void_t<>>
struct has_emplace_after : public std::false_type {};

template<typename T>
struct has_emplace_after<T,
                         std::void_t<decltype(std::declval<T&>().emplace_after(std::declval<typename T::const_iterator>(),
                                                                          std::declval<typename T::value_type>()))>>
    : public std::true_type {};

// has_before_begin
template<typename, typename = std::void_t<>>
struct has_before_begin : public std::false_type {};

template<typename T>
struct has_before_begin<
    T,
    std::void_t<std::enable_if_t<std::is_same<typename T::iterator, decltype(std::declval<T&>().before_begin())>::value>>>
    : public std::true_type {};

// is_like_sequence_container
template<typename T>
struct is_like_sequence_container {
    static constexpr bool value =
        has_value_type<T>::value && !has_key_type<T>::value && has_iterator_type<T>::value &&
        has_const_iterator_type<T>::value && has_begin<T>::value && has_end<T>::value &&
        (has_emplace_back<T>::value || (has_before_begin<T>::value && has_emplace_after<T>::value));
};

// is_like_associative_container
template<typename T>
struct is_like_associative_container {
    static constexpr bool value = has_value_type<T>::value && has_key_type<T>::value && has_iterator_type<T>::value &&
                                  has_const_iterator_type<T>::value && has_begin<T>::value && has_end<T>::value &&
                                  has_emplace<T>::value;
};

// is_map_like_associative_container
template<typename T>
struct is_map_like_associative_container {
    static constexpr bool value = is_like_associative_container<T>::value && has_mapped_type<T>::value;
};

// is_set_like_associative_container
template<typename T>
struct is_set_like_associative_container {
    static constexpr bool value = is_like_associative_container<T>::value && !has_mapped_type<T>::value;
};

// is_like_container
template<typename T>
struct is_like_container {
    static constexpr bool value =
        is_like_sequence_container<T>::value || is_like_associative_container<T>::value;
};

template<typename T>
constexpr inline bool is_like_container_v = is_like_container<T>::value;

} // namespace components::serialization::traits