#pragma once

// clang-format off
#include <components/serialization/context.hpp>
// clang-format on

namespace components::serialization {

    template<class Storage>
    void serialize_array(context::context_t<Storage>& context, std::size_t size);

    template<class Storage>
    void serialize_map(context::context_t<Storage>& context, std::size_t size);

    template<class Storage, class Object>
    void serialize(context::context_t<Storage>& context, Object& data);

    template<class Storage>
    void deserialize_array(context::context_t<Storage>& context, std::size_t size);
    template<class Storage>
    void deserialize_map(context::context_t<Storage>& context, std::size_t size);

    template<class Storage, class Object>
    void deserialize(context::context_t<Storage>& context, Object& data);

} // namespace components::serialization