#pragma once

#include <components/cursor/cursor.hpp>
#include <components/log/log.hpp>
#include <components/logical_plan/node.hpp>
#include <components/ql/ql_param_statement.hpp>
#include <components/session/session.hpp>
#include <core/btree/btree.hpp>
#include <core/excutor.hpp>
#include <core/spinlock/spinlock.hpp>
#include <memory_resource>
#include <services/collection/executor.hpp>
#include <services/collection/operators/operator.hpp>
#include <services/disk/result.hpp>
#include <stack>

#include <services/wal/base.hpp>
#include <services/wal/record.hpp>

#include "context_storage.hpp"
#include "session.hpp"

namespace components::ql {
    struct create_database_t;
    struct drop_database_t;
    struct create_collection_t;
    struct drop_collection_t;
} // namespace components::ql

namespace services {

    class memory_storage_t final : public actor_zeta::cooperative_supervisor<memory_storage_t> {
        struct load_buffer_t {
            std::pmr::vector<collection_full_name_t> collections;

            explicit load_buffer_t(std::pmr::memory_resource* resource);
        };

        using database_storage_t = std::pmr::set<database_name_t>;
        using collection_storage_t =
            core::pmr::btree::btree_t<collection_full_name_t, std::unique_ptr<collection::context_collection_t>>;
        using session_storage_t = core::pmr::btree::btree_t<components::session::session_id_t, session_t>;

    public:
        using address_pack = std::tuple<actor_zeta::address_t, actor_zeta::address_t, actor_zeta::address_t>;
        enum class unpack_rules : uint64_t
        {
            dispatcher = 0,
            manager_disk = 1,
            manager_wal = 2
        };

        memory_storage_t(actor_zeta::detail::pmr::memory_resource* resource,
                         actor_zeta::scheduler_raw scheduler,
                         log_t& log);
        ~memory_storage_t();

        void sync(const address_pack& pack);
        void execute_ql(components::session::session_id_t& session,
                        components::ql::ql_statement_t* ql,
                        actor_zeta::base::address_t address);
        void execute_ql_finish(components::session::session_id_t& session, components::cursor::cursor_t_ptr cursor);

        void size(components::session::session_id_t& session,
                  const std::string& database_name,
                  const std::string& collection_name);
        void close_cursor(components::session::session_id_t& session, std::set<collection_full_name_t>&& collections);
        void load(components::session::session_id_t& session);
        void load_from_disk_result(components::session::session_id_t& session, services::disk::result_load_t&& result);

        actor_zeta::scheduler_abstract_t* scheduler_impl() noexcept final;
        void enqueue_impl(actor_zeta::message_ptr msg, actor_zeta::execution_unit* unit) final;

    private:
        spin_lock lock_;
        actor_zeta::detail::pmr::memory_resource* resource_;
        actor_zeta::address_t dispatcher_{actor_zeta::address_t::empty_address()};
        actor_zeta::address_t manager_disk_{actor_zeta::address_t::empty_address()};
        actor_zeta::address_t manager_wal_{actor_zeta::address_t::empty_address()};
        actor_zeta::address_t executor_address_{actor_zeta::address_t::empty_address()};
        services::collection::executor::executor_t* executor_ = nullptr;
        log_t log_;
        actor_zeta::scheduler_raw e_;
        database_storage_t databases_;
        collection_storage_t collections_;
        session_storage_t sessions_;
        std::unique_ptr<load_buffer_t> load_buffer_;

        disk::result_load_t load_result_;
        components::session::session_id_t load_session_;
        std::size_t load_count_answers_{0};
        services::wal::id_t last_wal_id_{0};
        std::unordered_map<components::session::session_id_t, components::cursor::cursor_t_ptr>
            result_storage_; // to be able return result from wal_success
        // TODO figure out what to do with records
        std::vector<services::wal::record_t> records_;

        std::pair<components::logical_plan::node_ptr, components::ql::storage_parameters>
        create_logic_plan(components::ql::ql_statement_t* statement);
        void wal_success(components::session::session_id_t& session, services::wal::id_t wal_id);
        void load_from_wal_result(components::session::session_id_t& session,
                                  std::vector<services::wal::record_t>& records);
        void load_from_disk_finished(components::session::session_id_t& session);

        bool is_exists_database_(const database_name_t& name) const;
        bool is_exists_collection_(const collection_full_name_t& name) const;
        bool check_database_(components::session::session_id_t& session, const database_name_t& name);
        bool check_collection_(components::session::session_id_t& session, const collection_full_name_t& name);

        // TODO separate change logic and condition check
        bool load_from_wal_in_progress(components::session::session_id_t& session);

        void create_database_(components::session::session_id_t& session,
                              components::logical_plan::node_ptr logical_plan);
        void drop_database_(components::session::session_id_t& session,
                            components::logical_plan::node_ptr logical_plan);
        void create_collection_(components::session::session_id_t& session,
                                components::logical_plan::node_ptr logical_plan);
        void drop_collection_(components::session::session_id_t& session,
                              components::logical_plan::node_ptr logical_plan);

        void execute_plan_(components::session::session_id_t& session,
                           components::logical_plan::node_ptr logical_plan,
                           components::ql::storage_parameters parameters);
    };

} // namespace services
