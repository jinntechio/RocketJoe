#pragma once

// clang-format off
#include <components/serialization/context.hpp>
#include <components/serialization/traits.hpp>
#include <components/serialization/json/deserialization_json.hpp>
// clang-format on

namespace components::serialization {

    template<class Storage>
    void deserialize_array(context::context_t<Storage>& context, std::size_t size) {
        detail::intermediate_deserialize_array(context, size, 0);
    }

    template<class Storage>
    void deserialize_map(context::context_t<Storage>& context, std::size_t size) {
        detail::intermediate_deserialize_map(context, size, 0);
    }

    template<class Storage, class Object>
    void deserialize(context::context_t<Storage>& context, Object& data) {
        if constexpr (traits::is_user_class_v<Object>) {
           deserialize(context, data.access_as_tuple());
        } else if constexpr (traits::is_tuple_v<Object>) {
            std::apply([&context](auto&&... args) { (deserialize(context, args), ...); }, data);
        } else {
            detail::intermediate_deserialize(context,
                                             data,
                                             0,
                                             typename traits::serialization_trait<Object>::category{});
        }
    }
} // namespace components::serialization
