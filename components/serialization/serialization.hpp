#pragma once

// clang-format off
#include <components/serialization/context.hpp>
#include <components/serialization/traits.hpp>
#include <components/serialization/json/serialization_json.hpp>
// clang-format on

namespace components::serialization {

    template<class Storage>
    void serialize_array(context::context_t<Storage>& context, std::size_t size) {
        detail::intermediate_serialize_array(context, size, 0);
    }

    template<class Storage>
    void serialize_map(context::context_t<Storage>& context, std::size_t size) {
        detail::intermediate_serialize_map(context, size, 0);
    }

    template<class Storage, class Object>
    void serialize(context::context_t<Storage>& context, Object& data) {
        if constexpr (traits::is_user_class_v<Object>) {
             serialize(context,data.access_as_tuple());
        } else if constexpr (traits::is_tuple_v<Object>) {
            std::apply([&context](auto&&... args) { (serialize(context, args), ...); }, data);
        } else {
            detail::intermediate_serialize(context, data, 0, typename traits::serialization_trait<Object>::category{});
        }
    }
} // namespace components::serialization
