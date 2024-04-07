#pragma once

#include <components/serialization/context.hpp>

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>

namespace components::serialization::context {
    namespace detail {

        enum class state_t { init, array, object };

    } // namespace detail

    template<>
    class context_t<boost::json::value> {
    public:
        context_t() = default;
        ~context_t() = default;
        context_t(const std::string& str): value_(boost::json::parse(str)) {}
        context_t(boost::json::value& data): value_(data) {}
        [[nodiscard]] std::string data() const { return boost::json::serialize(value_); }

        detail::state_t state_{detail::state_t::init};
        std::size_t number_of_elements{0};
        std::size_t index_serialization{0};
        std::size_t index_deserialization{0};
    private:
        boost::json::value value_;
    };

    using json_context = context_t<boost::json::value>;
} // namespace components::serialization::context
