#include "convert.hpp"
#include "deserialization_json.hpp"
#include "serialization_json.hpp"

#include <string>
#include <string_view>
namespace {
    void
    make_array(components::serialization::context::json_context& context, std::size_t size, const unsigned int version) {
        boost::ignore_unused(version);
        assert(components::serialization::context::detail::state_t::init == context.state_);
        ///assert(size==0);
        context.number_of_elements = size;
        context.index_serialization = size;
        context.index_deserialization = 0;
        context.state_ = components::serialization::context::detail::state_t::array;
    }

    void
    make_map(components::serialization::context::json_context& context, std::size_t size, const unsigned int version) {
        boost::ignore_unused(version);
        assert(components::serialization::context::detail::state_t::init == context.state_);
        ///assert(size==0);
        context.number_of_elements = size;
        context.index_serialization = size;
        context.index_deserialization = 0;
        context.state_ = components::serialization::context::detail::state_t::object;
    }

} // namespace

namespace components::serialization::detail {

    void intermediate_serialize_array(context::json_context& context, std::size_t size, const unsigned int version) {
        make_array(context, size, version);
        context.value_ = boost::json::array();
    }

    void intermediate_serialize_map(context::json_context& context, std::size_t size, const unsigned int version) {
        make_map(context, size, version);
        context.value_ = boost::json::object();
    }

    void intermediate_deserialize_array(context::json_context& ar, std::size_t size, const unsigned int version) {
        make_array(ar, size, version);
    }

    void intermediate_deserialize_map(context::json_context& ar, std::size_t size, const unsigned int version) {
        make_map(ar, size, version);
    }

} // namespace components::serialization::detail

namespace components::serialization::detail {

    std::string convert(const std::string& t) { return std::string(t); }

    std::string convert(std::string_view t) { return std::string(t.data(), t.size()); }

} // namespace components::serialization::detail