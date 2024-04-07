#pragma once

#include <cassert>
#include <iostream>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>

#include <components/serialization/json/context_json.hpp>
#include <components/serialization/json/convert.hpp>
#include <components/serialization/traits.hpp>

#include <components/serialization/forward.hpp>

namespace components::serialization::detail {

    void intermediate_deserialize_array(context::json_context& context, std::size_t size, const unsigned int version);
    void intermediate_deserialize_map(context::json_context& context, std::size_t size, const unsigned int version);

    template<typename Contaner>
    void intermediate_deserialize(context::json_context& context,
                               Contaner& data,
                               const unsigned int version,
                               traits::signed_number_tag) {
        boost::ignore_unused(version);
        assert(context.index_deserialization >= 0 && context.index_deserialization < context.number_of_elements);
        assert(context::detail::state_t::array == context.state_);
        if constexpr (std::is_same_v<int8_t, std::decay_t<Contaner>>) {
            data = static_cast<int8_t>(context.value_.as_array()[context.index_deserialization].as_int64());
        } else if constexpr (std::is_same_v<int16_t, std::decay_t<Contaner>>) {
            data = static_cast<int16_t>(context.value_.as_array()[context.index_deserialization].as_int64());
        } else if constexpr (std::is_same_v<int32_t, std::decay_t<Contaner>>) {
            data = static_cast<int32_t>(context.value_.as_array()[context.index_deserialization].as_int64());
        } else if constexpr (std::is_same_v<int64_t, std::decay_t<Contaner>>) {
            const auto& tmp = context.value_.as_array()[context.index_deserialization].as_int64();
            data = tmp;
        }
        ++context.index_deserialization;
    }

    template<typename Contaner>
    void intermediate_deserialize(context::json_context& context,
                               Contaner& data,
                               const unsigned int version,
                               traits::unsigned_number_tag) {
        boost::ignore_unused(version);
        assert(context.index_deserialization >= 0 && context.index_deserialization < context.number_of_elements);
        assert(context::detail::state_t::array == context.state_);
        if constexpr (std::is_same_v<uint8_t, std::decay_t<Contaner>>) {
            data = static_cast<uint8_t>(context.value_.as_array()[context.index_deserialization].as_uint64());
        } else if constexpr (std::is_same_v<uint16_t, std::decay_t<Contaner>>) {
            data = static_cast<uint16_t>(context.value_.as_array()[context.index_deserialization].as_uint64());
        } else if constexpr (std::is_same_v<uint32_t, std::decay_t<Contaner>>) {
            data = static_cast<uint32_t>(context.value_.as_array()[context.index_deserialization].as_uint64());
        } else if constexpr (std::is_same_v<uint64_t, std::decay_t<Contaner>>) {
            data = static_cast<uint64_t>(context.value_.as_array()[context.index_deserialization].as_uint64());
        }
        ++context.index_deserialization;
    }

    template<typename Contaner>
    void intermediate_deserialize(context::json_context& context,
                               Contaner& data,
                               const unsigned int version,
                               traits::char_tag) {
        boost::ignore_unused(version);
        assert(context.index_serialization > 0);
        assert(context::detail::state_t::array == context.state_);
        context.value_.as_array().emplace_back(boost::json::value(data));
        ++context.index_deserialization;
    }

    template<typename Contaner>
    void intermediate_deserialize(context::json_context& context,
                               Contaner& data,
                               const unsigned int version,
                               traits::boolean_tag) {
        boost::ignore_unused(version);
        assert(context.index_deserialization >= 0 && context.index_deserialization < context.number_of_elements);
        assert(context::detail::state_t::array == context.state_);
        data = context.value_.as_array()[context.index_deserialization].as_bool();
        ++context.index_deserialization;
    }

    template<class Contaner>
    void intermediate_deserialize(context::json_context& context,
                               Contaner& data,
                               const unsigned int version,
                               traits::string_tag) {
        boost::ignore_unused(version);
        assert(context.index_deserialization >= 0 && context.index_deserialization < context.number_of_elements);
        assert(context::detail::state_t::array == context.state_);
        const auto& str = context.value_.as_array()[context.index_deserialization].as_string();
        auto first = std::begin(str);
        auto last = std::end(str);
        std::copy(first,last,std::back_inserter(data));
        ++context.index_deserialization;
    }

    template<class T>
    void intermediate_deserialize(context::json_context& context,
                               std::vector<T>& data,
                               const unsigned int version,
                               traits::array_tag) {
        std::cerr << "intermediate_deserialize array" << std::endl;
        boost::ignore_unused(version);
        assert(context.index_deserialization >= 0 && context.index_deserialization < context.number_of_elements);
        assert(context::detail::state_t::array == context.state_);
        const auto& array = context.value_.as_array()[context.index_deserialization].as_array();
        for(auto&i:array) {
            std::cerr << typeid(i).name() << std::endl;

            context::json_context tmp(i);
            T tmp_data;
            auto size = 0;
            switch (i.kind()) {
                case boost::json::kind::int64:
                    size=1;
                    break;
                case boost::json::kind::uint64:
                    size=1;
                    break;
                case boost::json::kind::bool_:
                    size=1;
                    break;
                case boost::json::kind::string:
                    size=1;
                    break;
                case boost::json::kind::array:
                    size=i.as_array().size();
                    break;
                case boost::json::kind::object:
                    size=i.as_object().size();
                    break;
                case boost::json::kind::null:
                    size=0;
            }
            deserialize_array(tmp,size);
            deserialize(tmp,tmp_data);
            data.emplace_back(std::move(tmp_data));
        }
        ++context.index_deserialization;
    }

    template<class Contaner>
    void intermediate_deserialize(context::json_context& context,
                               Contaner& data,
                               const unsigned int version,
                               traits::object_tag) {
        boost::ignore_unused(version);
        assert(context.index_serialization > 0);
        assert(context::detail::state_t::array == context.state_);
        auto size = std::size(data);
        auto first = std::begin(data);
        auto last = std::end(data);
        boost::json::object object(size, context.value_.get_allocator().resource());
        for (auto it = first; it != last; ++it) {
            const auto& [key, value] = *it;
            object.emplace(detail::convert(key), value); /// TODO: value is simple type
        }
        context.value_.as_array().emplace_back(std::move(object));
        ++context.index_deserialization;
    }

} // namespace components::serialization::detail