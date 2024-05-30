#pragma once

#include <actor-zeta.hpp>

#include <boost/filesystem.hpp>
#include <log/log.hpp>

#include <components/ql/statements.hpp>
#include <components/session/session.hpp>
#include <configuration/configuration.hpp>
#include <core/excutor.hpp>
#include <core/spinlock/spinlock.hpp>

#include "base.hpp"
#include "route.hpp"
#include "wal.hpp"

namespace services::wal {

    class base_manager_wal_replicate_t : public actor_zeta::cooperative_supervisor<base_manager_wal_replicate_t> {
    protected:
        base_manager_wal_replicate_t(actor_zeta::detail::pmr::memory_resource* mr, actor_zeta::scheduler_raw scheduler);

    private:
        actor_zeta::scheduler_raw e_;
        spin_lock lock_;

        auto scheduler_impl() noexcept -> actor_zeta::scheduler_abstract_t* final;
        auto enqueue_impl(actor_zeta::message_ptr msg, actor_zeta::execution_unit*) -> void final;
    };

    using manager_wal_ptr = std::unique_ptr<base_manager_wal_replicate_t>;

    class manager_wal_replicate_t final : public base_manager_wal_replicate_t {
        using session_id_t = components::session::session_id_t;

    public:
        using address_pack = std::tuple<actor_zeta::address_t, actor_zeta::address_t>;

        enum class unpack_rules : uint64_t
        {
            manager_disk = 0,
            memory_storage = 1
        };

        void sync(address_pack& pack) {
            manager_disk_ = std::get<static_cast<uint64_t>(unpack_rules::manager_disk)>(pack);
            memory_storage_ = std::get<static_cast<uint64_t>(unpack_rules::memory_storage)>(pack);
        }

        manager_wal_replicate_t(actor_zeta::detail::pmr::memory_resource*,
                                actor_zeta::scheduler_raw,
                                configuration::config_wal,
                                log_t&);
        ~manager_wal_replicate_t() final;
        void create_wal_worker();
        void load(session_id_t& session, services::wal::id_t wal_id);
        void create_database(session_id_t& session, components::ql::create_database_t& data);
        void drop_database(session_id_t& session, components::ql::drop_database_t& data);
        void create_collection(session_id_t& session, components::ql::create_collection_t& data);
        void drop_collection(session_id_t& session, components::ql::drop_collection_t& data);
        void insert_one(session_id_t& session, components::ql::insert_one_t& data);
        void insert_many(session_id_t& session, components::ql::insert_many_t& data);
        void delete_one(session_id_t& session, components::ql::delete_one_t& data);
        void delete_many(session_id_t& session, components::ql::delete_many_t& data);
        void update_one(session_id_t& session, components::ql::update_one_t& data);
        void update_many(session_id_t& session, components::ql::update_many_t& data);
        void create_index(session_id_t& session, components::ql::create_index_t& data);

    private:
        actor_zeta::address_t manager_disk_ = actor_zeta::address_t::empty_address();
        actor_zeta::address_t memory_storage_ = actor_zeta::address_t::empty_address();
        configuration::config_wal config_;
        log_t log_;
        std::unordered_map<std::string, actor_zeta::address_t> dispatcher_to_address_book_;
        std::vector<wal_replicate_ptr> dispatchers_;
    };

    class manager_wal_replicate_empty_t final : public base_manager_wal_replicate_t {
        using session_id_t = components::session::session_id_t;
        using address_pack = std::tuple<actor_zeta::address_t, actor_zeta::address_t>;

    public:
        manager_wal_replicate_empty_t(actor_zeta::detail::pmr::memory_resource*, actor_zeta::scheduler_raw, log_t&);

        template<class T>
        auto always_success(session_id_t& session, T&&) -> void {
            trace(log_, "manager_wal_replicate_empty_t::always_success: session: {}", session.data()); // Works fine
            actor_zeta::send(current_message()->sender(),
                             address(),
                             services::wal::handler_id(services::wal::route::success),
                             session,
                             services::wal::id_t(0));
            trace(log_, "manager_wal_replicate_empty_t::always_success: sent finished"); // is not called
        }

        template<class... Args>
        auto nothing(Args&&...) -> void {}

    private:
        log_t log_;
    };

} //namespace services::wal
