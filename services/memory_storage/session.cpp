#include "session.hpp"

#include <components/ql/statements.hpp>

using namespace components::ql;

session_t::session_t(actor_zeta::address_t address)
    : address_(std::move(address))
    , data_(unused_statement_t()) {}

session_t::session_t(actor_zeta::address_t address, size_t count_answers)
    : address_(std::move(address))
    , data_(unused_statement_t())
    , count_answers_(count_answers) {}

statement_type session_t::type() const {
    return std::visit(
        [](const auto& c) {
            using type = std::decay_t<decltype(c)>;
            if constexpr (std::is_same_v<type, components::ql::unused_statement_t>) {
                return statement_type::unused;
            } else if constexpr (std::is_same_v<type, components::ql::create_database_t>) {
                return statement_type::create_database;
            } else if constexpr (std::is_same_v<type, components::ql::drop_database_t>) {
                return statement_type::drop_database;
            } else if constexpr (std::is_same_v<type, components::ql::create_collection_t>) {
                return statement_type::create_collection;
            } else if constexpr (std::is_same_v<type, components::ql::drop_collection_t>) {
                return statement_type::drop_collection;
            } else if constexpr (std::is_same_v<type, components::ql::insert_one_t>) {
                return statement_type::insert_one;
            } else if constexpr (std::is_same_v<type, insert_many_t>) {
                return statement_type::insert_many;
            } else if constexpr (std::is_same_v<type, aggregate_statement>) {
                return statement_type::aggregate;
            } else if constexpr (std::is_same_v<type, delete_one_t>) {
                return statement_type::delete_one;
            } else if constexpr (std::is_same_v<type, delete_many_t>) {
                return statement_type::delete_many;
            } else if constexpr (std::is_same_v<type, update_one_t>) {
                return statement_type::update_one;
            } else if constexpr (std::is_same_v<type, update_many_t>) {
                return statement_type::update_many;
            } else if constexpr (std::is_same_v<type, create_index_t>) {
                return statement_type::create_index;
            } else if constexpr (std::is_same_v<type, drop_index_t>) {
                return statement_type::drop_index;
            } else if constexpr (std::is_same_v<type, join_t>) {
                return statement_type::join;
            } else if constexpr (std::is_same_v<type, ql_statement_t*>) {
                return statement_type::unused;
            }
            static_assert(true, "Not valid command type");
        },
        data_);
}
