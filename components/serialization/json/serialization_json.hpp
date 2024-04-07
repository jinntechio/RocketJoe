#pragma once

#include <cassert>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>

#include <components/serialization/traits.hpp>
#include <components/serialization/json/context_json.hpp>
#include <components/serialization/json/convert.hpp>

namespace components::serialization::detail {

    void intermediate_serialize_array(context::json_context& context, std::size_t size, const unsigned int version);
    void intermediate_serialize_map(context::json_context& context, std::size_t size, const unsigned int version);

    template<typename Contaner>
    void intermediate_serialize(context::json_context& context,
                                Contaner& data,
                                const unsigned int version,
                                traits::signed_number_tag) {
        boost::ignore_unused(version);
        assert(context.index_serialization > 0);
        assert(context::detail::state_t::array == context.state_);
        context.value_.as_array().emplace_back(boost::json::value(data));
        context.index_serialization--;
    }

    template<typename Contaner>
    void intermediate_serialize(context::json_context& context,
                                Contaner& data,
                                const unsigned int version,
                                traits::unsigned_number_tag) {
        boost::ignore_unused(version);
        assert(context.index_serialization > 0);
        assert(context::detail::state_t::array == context.state_);
        context.value_.as_array().emplace_back(boost::json::value(data));
        context.index_serialization--;
    }

    template<typename Contaner>
    void intermediate_serialize(context::json_context& context,
                                Contaner& data,
                                const unsigned int version,
                                traits::char_tag) {
        boost::ignore_unused(version);
        assert(context.index_serialization > 0);
        assert(context::detail::state_t::array == context.state_);
        context.value_.as_array().emplace_back(boost::json::value(data));
        context.index_serialization--;
    }

    template<typename Contaner>
    void intermediate_serialize(context::json_context& context,
                                Contaner& data,
                                const unsigned int version,
                                traits::boolean_tag) {
        boost::ignore_unused(version);
        assert(context.index_serialization > 0);
        assert(context::detail::state_t::array == context.state_);
        context.value_.as_array().emplace_back(boost::json::value(data));
        context.index_serialization--;
    }


    template<class Contaner>
    void
    intermediate_serialize(context::json_context& context, Contaner& data, const unsigned int version, traits::string_tag) {
        boost::ignore_unused(version);
        assert(context.index_serialization > 0);
        assert(context::detail::state_t::array == context.state_);
        auto size = std::size(data);
        auto first = std::begin(data);
        auto last = std::end(data);
        boost::json::string string(context.value_.get_allocator().resource());
        string.assign(first, last);
        context.value_.as_array().emplace_back(std::move(string));
        context.index_serialization--;
    }

    template<class Contaner>
    void intermediate_serialize(context::json_context& context,
                                Contaner& data,
                                const unsigned int version,
                                traits::array_tag) {
        boost::ignore_unused(version);
        assert(context.index_serialization > 0);
        assert(context::detail::state_t::array == context.state_);
        boost::json::array array(context.value_.get_allocator().resource());
        auto size = std::size(data);
        auto first = std::begin(data);
        auto last = std::end(data);
        array.reserve(size);
        for (; first != last; ++first) {
            array.emplace_back(*first);
        }
        context.value_.as_array().emplace_back(std::move(array));
        context.index_serialization--;
    }

    template<class Contaner>
    void
    intermediate_serialize(context::json_context& context, Contaner& data, const unsigned int version, traits::object_tag) {
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
        context.index_serialization--;
    }

} // namespace components::serialization::detail