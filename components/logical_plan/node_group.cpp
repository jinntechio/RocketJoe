#include "node_group.hpp"
#include <sstream>

namespace components::logical_plan {

    node_group_t::node_group_t(const ql::aggregate::group_t& group)
        : node_t(node_type::group_t) {
        for (const auto& expr : group.fields) {
            append_expression(expr);
        }
    }

    hash_t node_group_t::hash_impl() const {
        return 0;
    }

    std::string node_group_t::to_string_impl() const {
        std::stringstream stream;
        stream << "$group: {";
        bool is_first = true;
        for (const auto& expr : expressions_) {
            if (is_first) {
                is_first = false;
            } else {
                stream << ", ";
            }
            stream << expr->to_string();
        }
        stream << "}";
        return stream.str();
    }


    node_ptr make_node_group(const ql::aggregate::group_t& group) {
        return new node_group_t(group);
    }

}