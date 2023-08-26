#pragma once

#include <memory_resource>
#include <core/btree/btree.hpp>
#include <core/excutor.hpp>
#include <core/spinlock/spinlock.hpp>
#include <components/log/log.hpp>
#include <components/ql/base.hpp>
#include <components/ql/ql_statement.hpp>
#include <components/session/session.hpp>
#include <services/collection/result.hpp>
#include <services/disk/result.hpp>
#include "result.hpp"

namespace components::ql {
    struct create_database_t;
    struct drop_database_t;
    struct create_collection_t;
    struct drop_collection_t;
}

namespace services {

    class memory_storage_t final : public actor_zeta::cooperative_supervisor<memory_storage_t> {
        struct session_t {
            components::ql::ql_statement_t* ql;
            actor_zeta::address_t sender;
            size_t count_answers;
        };

        struct load_buffer_t {
            memory_storage::result_list_addresses_t collections;

            explicit load_buffer_t(std::pmr::memory_resource* resource);
        };

        using database_storage_t = std::pmr::set<database_name_t>;
        using collection_storage_t = core::pmr::btree::btree_t<collection_full_name_t, actor_zeta::address_t>;
        using session_storage_t = core::pmr::btree::btree_t<components::session::session_id_t, session_t>;

    public:
        using address_pack = std::tuple<actor_zeta::address_t, actor_zeta::address_t>;
        enum class unpack_rules : uint64_t {
            manager_dispatcher = 0,
            manager_disk = 1
        };

        memory_storage_t(actor_zeta::detail::pmr::memory_resource* resource, actor_zeta::scheduler_raw scheduler, log_t& log);
        ~memory_storage_t();

        void sync(const address_pack& pack);
        void execute_ql(components::session::session_id_t& session, components::ql::ql_statement_t* ql);
        void load(components::session::session_id_t &session, const disk::result_load_t &result);

        actor_zeta::scheduler_abstract_t* scheduler_impl() noexcept final;
        void enqueue_impl(actor_zeta::message_ptr msg, actor_zeta::execution_unit* unit) final;

    private:
        spin_lock lock_;
        actor_zeta::address_t manager_dispatcher_{actor_zeta::address_t::empty_address()};
        actor_zeta::address_t manager_disk_{actor_zeta::address_t::empty_address()};
        log_t log_;
        actor_zeta::scheduler_raw e_;
        database_storage_t databases_;
        collection_storage_t collections_;
        session_storage_t sessions_;
        std::unique_ptr<load_buffer_t> load_buffer_;

        bool is_exists_database(const database_name_t& name) const;
        bool is_exists_collection(const collection_full_name_t& name) const;

        void create_database_(components::session::session_id_t& session, components::ql::create_database_t* ql);
        void drop_database_(components::session::session_id_t& session, components::ql::drop_database_t* ql);
        void create_collection_(components::session::session_id_t& session, components::ql::create_collection_t* ql);
        void drop_collection_(components::session::session_id_t& session, components::ql::drop_collection_t* ql);

        void drop_collection_finish_(components::session::session_id_t& session, result_drop_collection& result);
        void create_documents_finish_(components::session::session_id_t& session);
    };

} // namespace services