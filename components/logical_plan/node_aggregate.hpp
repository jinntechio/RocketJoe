#pragma once

#include <components/ql/aggregate.hpp>
#include "node.hpp"

namespace components::logical_plan {

    class node_aggregate_t final : public node_t {
    public:
        node_aggregate_t();

    private:
        hash_t hash_impl() const final;
        std::string to_string_impl() const final;
    };

}