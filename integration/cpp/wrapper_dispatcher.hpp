#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <variant>

#include <actor-zeta.hpp>

#include <core/spinlock/spinlock.hpp>

#include <components/cursor/cursor.hpp>
#include <components/document/document.hpp>
#include <components/log/log.hpp>
#include <components/ql/aggregate.hpp>
#include <components/ql/index.hpp>
#include <components/ql/statements.hpp>
#include <components/session/session.hpp>

namespace otterbrix {

    using components::document::document_ptr;
    using components::session::session_id_t;

    class wrapper_dispatcher_t final : public actor_zeta::cooperative_supervisor<wrapper_dispatcher_t> {
    public:
        /// blocking method
        wrapper_dispatcher_t(actor_zeta::detail::pmr::memory_resource*, actor_zeta::address_t, log_t& log);
        ~wrapper_dispatcher_t();
        auto load() -> void;
        [[deprecated]] auto create_database(session_id_t& session, const database_name_t& database)
            -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto drop_database(session_id_t& session, const database_name_t& database)
            -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto create_collection(session_id_t& session,
                                              const database_name_t& database,
                                              const collection_name_t& collection) -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto drop_collection(session_id_t& session,
                                            const database_name_t& database,
                                            const collection_name_t& collection) -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto insert_one(session_id_t& session,
                                       const database_name_t& database,
                                       const collection_name_t& collection,
                                       document_ptr& document) -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto insert_many(session_id_t& session,
                                        const database_name_t& database,
                                        const collection_name_t& collection,
                                        std::pmr::vector<document_ptr>& documents) -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto find(session_id_t& session, components::ql::aggregate_statement_raw_ptr condition)
            -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto find_one(session_id_t& session, components::ql::aggregate_statement_raw_ptr condition)
            -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto delete_one(session_id_t& session, components::ql::aggregate_statement_raw_ptr condition)
            -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto delete_many(session_id_t& session, components::ql::aggregate_statement_raw_ptr condition)
            -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto update_one(session_id_t& session,
                                       components::ql::aggregate_statement_raw_ptr condition,
                                       document_ptr update,
                                       bool upsert) -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto update_many(session_id_t& session,
                                        components::ql::aggregate_statement_raw_ptr condition,
                                        document_ptr update,
                                        bool upsert) -> components::cursor::cursor_t_ptr;
        [[deprecated]] auto
        size(session_id_t& session, const database_name_t& database, const collection_name_t& collection) -> size_t;
        auto create_index(session_id_t& session, components::ql::create_index_t* ql)
            -> components::cursor::cursor_t_ptr;
        auto drop_index(session_id_t& session, components::ql::drop_index_t* ql) -> components::cursor::cursor_t_ptr;
        auto execute_ql(session_id_t& session, components::ql::variant_statement_t& query)
            -> components::cursor::cursor_t_ptr;
        auto execute_sql(session_id_t& session, const std::string& query) -> components::cursor::cursor_t_ptr;

    protected:
        auto scheduler_impl() noexcept -> actor_zeta::scheduler_abstract_t* final;
        auto enqueue_impl(actor_zeta::message_ptr msg, actor_zeta::execution_unit*) -> void final;

    private:
        /// async method
        auto load_finish() -> void;
        auto execute_ql_finish(session_id_t& session, components::cursor::cursor_t_ptr cursor) -> void;
        auto size_finish(session_id_t& session, size_t size) -> void;

        void init();
        void wait();
        void notify();

        auto send_ql(session_id_t& session, components::ql::ql_statement_t* ql) -> components::cursor::cursor_t_ptr;

        actor_zeta::address_t memory_storage_;
        log_t log_;
        std::atomic_int i = 0;
        std::mutex output_mtx_;
        spin_lock input_mtx_;
        std::condition_variable cv_;
        session_id_t input_session_;
        components::cursor::cursor_t_ptr cursor_store_;
        size_t size_store_;
        bool bool_store_;
    };
} // namespace otterbrix
