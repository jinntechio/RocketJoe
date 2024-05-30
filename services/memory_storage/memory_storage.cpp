#include "memory_storage.hpp"
#include "route.hpp"
#include <cassert>
#include <components/ql/statements/create_collection.hpp>
#include <components/ql/statements/create_database.hpp>
#include <components/ql/statements/drop_collection.hpp>
#include <components/ql/statements/drop_database.hpp>
#include <components/translator/ql_translator.hpp>
#include <core/system_command.hpp>
#include <core/tracy/tracy.hpp>
#include <services/collection/collection.hpp>
#include <services/collection/planner/create_plan.hpp>
#include <services/disk/route.hpp>
#include <services/wal/route.hpp>

using namespace components::cursor;

namespace services {

    using namespace services::memory_storage;
    using namespace core::pmr;
    using namespace components::ql;

    memory_storage_t::load_buffer_t::load_buffer_t(std::pmr::memory_resource* resource)
        : collections(resource) {}

    memory_storage_t::memory_storage_t(actor_zeta::detail::pmr::memory_resource* resource,
                                       actor_zeta::scheduler_raw scheduler,
                                       log_t& log)
        : actor_zeta::cooperative_supervisor<memory_storage_t>(resource, "memory_storage")
        , resource_(resource)
        , log_(log.clone())
        , e_(scheduler)
        , databases_(resource)
        , collections_(resource) {
        ZoneScoped;
        trace(log_, "memory_storage start thread pool");
        executor_address_ = spawn_actor<services::collection::executor::executor_t>(
            [this](services::collection::executor::executor_t* ptr) { executor_ = ptr; },
            resource,
            std::move(log_.clone()));
        add_handler(core::handler_id(core::route::sync), &memory_storage_t::sync);
        add_handler(memory_storage::handler_id(memory_storage::route::execute_ql), &memory_storage_t::execute_ql);

        add_handler(collection::handler_id(collection::route::execute_plan_finish),
                    &memory_storage_t::execute_ql_finish);
        add_handler(collection::handler_id(collection::route::size), &memory_storage_t::size);
        add_handler(collection::handler_id(collection::route::close_cursor), &memory_storage_t::close_cursor);

        add_handler(core::handler_id(core::route::load), &memory_storage_t::load);
        add_handler(disk::handler_id(disk::route::load_finish), &memory_storage_t::load_from_disk_result);
        add_handler(wal::handler_id(wal::route::load_finish), &memory_storage_t::load_from_wal_result);
        add_handler(wal::handler_id(wal::route::success), &memory_storage_t::wal_success);
    }

    memory_storage_t::~memory_storage_t() {
        ZoneScoped;
        trace(log_, "delete memory_resource");
    }

    void memory_storage_t::sync(const address_pack& pack) {
        dispatcher_ = std::get<static_cast<uint64_t>(unpack_rules::dispatcher)>(pack);
        manager_disk_ = std::get<static_cast<uint64_t>(unpack_rules::manager_disk)>(pack);
        manager_wal_ = std::get<static_cast<uint64_t>(unpack_rules::manager_wal)>(pack);
    }

    void memory_storage_t::execute_ql(components::session::session_id_t& session,
                                      components::ql::ql_statement_t* ql,
                                      actor_zeta::base::address_t address) {
        sessions_.emplace(session, session_t{address, ql});
        auto logic_plan = create_logic_plan(ql);
        using components::logical_plan::node_type;

        switch (logic_plan.first->type()) {
            case node_type::create_database_t:
                create_database_(session, std::move(logic_plan.first));
                break;
            case node_type::drop_database_t:
                drop_database_(session, std::move(logic_plan.first));
                break;
            case node_type::create_collection_t:
                create_collection_(session, std::move(logic_plan.first));
                break;
            case node_type::drop_collection_t:
                drop_collection_(session, std::move(logic_plan.first));
                break;
            default:
                execute_plan_(session, std::move(logic_plan.first), std::move(logic_plan.second));
                break;
        }
    }

    void memory_storage_t::execute_ql_finish(components::session::session_id_t& session, cursor_t_ptr result) {
        result_storage_[session] = result;
        auto& s = sessions_.at(session);
        auto* ql = s.get<ql_statement_t*>();
        trace(log_,
              "memory_storage_t::execute_ql_finish: session {}, {}, {}",
              session.data(),
              ql->to_string(),
              result->is_success());

        if (result->is_success()) {
            switch (ql->type()) {
                case statement_type::create_database: {
                    actor_zeta::send(manager_disk_,
                                     address(),
                                     disk::handler_id(disk::route::append_database),
                                     session,
                                     ql->database_);
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::create_database),
                                         session,
                                         *static_cast<create_database_t*>(ql));
                        return;
                    }
                    break;
                }

                case statement_type::drop_database: {
                    actor_zeta::send(manager_disk_,
                                     address(),
                                     disk::handler_id(disk::route::remove_database),
                                     session,
                                     ql->database_);
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::drop_database),
                                         session,
                                         *static_cast<drop_database_t*>(ql));
                        return;
                    }
                    break;
                }

                case statement_type::create_collection: {
                    actor_zeta::send(manager_disk_,
                                     address(),
                                     disk::handler_id(disk::route::append_collection),
                                     session,
                                     ql->database_,
                                     ql->collection_);
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::create_collection),
                                         session,
                                         *static_cast<create_collection_t*>(ql));
                        return;
                    }
                    break;
                }

                case statement_type::drop_collection: {
                    collection_full_name_t name(ql->database_, ql->collection_);
                    actor_zeta::send(manager_disk_,
                                     address(),
                                     disk::handler_id(disk::route::remove_collection),
                                     session,
                                     ql->database_,
                                     ql->collection_);
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::drop_collection),
                                         session,
                                         components::ql::drop_collection_t(ql->database_, ql->collection_));
                        return;
                    }
                    break;
                }

                case statement_type::insert_one: {
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        assert(s.is_type<ql_statement_t*>() && "Doesn't holds ql_statement_t*");
                        auto statement = *static_cast<components::ql::insert_one_t*>(s.get<ql_statement_t*>());
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::insert_one),
                                         session,
                                         std::move(statement));
                        return;
                    }
                    break;
                }

                case statement_type::insert_many: {
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        assert(s.is_type<ql_statement_t*>() && "Doesn't holds ql_statement_t*");
                        auto statement = *static_cast<components::ql::insert_many_t*>(s.get<ql_statement_t*>());
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::insert_many),
                                         session,
                                         std::move(statement));
                        return;
                    }
                    break;
                }
                case statement_type::update_one: {
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        assert(s.is_type<ql_statement_t*>() && "Doesn't holds ql_statement_t*");
                        auto statement = *static_cast<components::ql::update_one_t*>(s.get<ql_statement_t*>());
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::update_one),
                                         session,
                                         std::move(statement));
                        return;
                    }
                    break;
                }

                case statement_type::update_many: {
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        assert(s.is_type<ql_statement_t*>() && "Doesn't holds ql_statement_t*");
                        auto statement = *static_cast<components::ql::update_many_t*>(s.get<ql_statement_t*>());
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::update_many),
                                         session,
                                         std::move(statement));
                        return;
                    }
                    break;
                }

                case statement_type::delete_one: {
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        assert(s.is_type<ql_statement_t*>() && "Doesn't holds ql_statement_t*");
                        auto statement = *static_cast<components::ql::delete_one_t*>(s.get<ql_statement_t*>());
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::delete_one),
                                         session,
                                         std::move(statement));
                        return;
                    }
                    break;
                }

                case statement_type::delete_many: {
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    } else {
                        assert(s.is_type<ql_statement_t*>() && "Doesn't holds ql_statement_t*");
                        auto statement = *static_cast<components::ql::delete_many_t*>(s.get<ql_statement_t*>());
                        actor_zeta::send(manager_wal_,
                                         address(),
                                         wal::handler_id(wal::route::delete_many),
                                         session,
                                         std::move(statement));
                        return;
                    }
                    break;
                }

                case statement_type::create_index:
                case statement_type::drop_index: {
                    if (s.address().get() == manager_wal_.get()) {
                        wal_success(session, last_wal_id_);
                    }
                    // We initiate session from this dispatcher,
                    // for now it means: dispatcher was called from manager_disk after initiate loading from disk.
                    if (s.address().get() == address().get()) {
                        sessions_.erase(session);
                        return;
                    }
                    break;
                }
            }
        }
        // TODO add verification for mutable types (they should be skipped too)
        if (!load_from_wal_in_progress(session)) {
            actor_zeta::send(s.address(), address(), handler_id(route::execute_ql_finish), session, std::move(result));
            sessions_.erase(session);
            result_storage_.erase(session);
        }
    }

    void memory_storage_t::size(components::session::session_id_t& session,
                                const std::string& database_name,
                                const std::string& collection_name) {
        collection_full_name_t name(database_name, collection_name);
        trace(log_, "memory_storage_t::size: collection: {}.{}", database_name, collection_name);
        if (!is_exists_collection_(name)) {
            actor_zeta::send(current_message()->sender(),
                             address(),
                             collection::handler_id(collection::route::size_finish),
                             session,
                             0);
            return;
        }
        auto collection = collections_.at(name).get();
        if (collection->dropped()) {
            actor_zeta::send(current_message()->sender(),
                             address(),
                             collection::handler_id(collection::route::size_finish),
                             session,
                             0);
        } else {
            actor_zeta::send(current_message()->sender(),
                             address(),
                             collection::handler_id(collection::route::size_finish),
                             session,
                             collection->storage().size());
        }
    }

    void memory_storage_t::close_cursor(components::session::session_id_t& session,
                                        std::set<collection_full_name_t>&& collections) {
        for (const auto& name : collections) {
            if (check_collection_(session, name)) {
                collections_.at(name)->cursor_storage().erase(session);
            }
        }
    }

    void memory_storage_t::load(components::session::session_id_t& session) {
        trace(log_, "memory_storage_t::load, session: {}", session.data());
        load_session_ = session;
        sessions_.emplace(session, session_t(current_message()->sender()));
        actor_zeta::send(manager_disk_, address(), disk::handler_id(disk::route::load), session);
    }

    void memory_storage_t::load_from_disk_result(components::session::session_id_t& session,
                                                 disk::result_load_t&& result) {
        load_result_ = std::move(result);
        trace(log_, "memory_storage_t:load_from_disk_result");
        load_buffer_ = std::make_unique<load_buffer_t>(resource());
        auto count_collections = std::accumulate(
            (*load_result_).begin(),
            (*load_result_).end(),
            0ul,
            [](size_t sum, const disk::result_database_t& database) { return sum + database.collections.size(); });
        if (count_collections > 0) {
            trace(log_, "memory_storage_t:load_from_disk_result: documents: {}", count_collections);
            sessions_.at(session).count_answers() = count_collections;
        }
        for (const auto& database : (*load_result_)) {
            debug(log_, "memory_storage_t:load:create_database: {}", database.name);
            databases_.insert(database.name);
            for (const auto& collection : database.collections) {
                debug(log_, "memory_storage_t:load_from_disk_result:create_collection: {}", collection.name);
                collection_full_name_t name(database.name, collection.name);
                auto context = new collection::context_collection_t(std::pmr::get_default_resource(),
                                                                    name,
                                                                    manager_disk_,
                                                                    log_.clone());
                collections_.emplace(name, context);
                load_buffer_->collections.emplace_back(name);
                debug(log_, "memory_storage_t:load_from_disk_result:fill_documents: {}", collection.documents.size());
                for (const auto& doc : collection.documents) {
                    context->storage().emplace(components::document::get_document_id(doc), doc);
                }
            }
        }
        load_from_disk_finished(session);
    }

    actor_zeta::scheduler::scheduler_abstract_t* memory_storage_t::scheduler_impl() noexcept { return e_; }

    void memory_storage_t::enqueue_impl(actor_zeta::message_ptr msg, actor_zeta::execution_unit*) {
        ZoneScoped;
        std::unique_lock<spin_lock> _(lock_);
        set_current_message(std::move(msg));
        execute(this, current_message());
    }

    std::pair<components::logical_plan::node_ptr, components::ql::storage_parameters>
    memory_storage_t::create_logic_plan(ql_statement_t* statement) {
        //todo: cache logical plans
        auto logic_plan = components::translator::ql_translator(resource_, statement);
        auto parameters = statement->is_parameters()
                              ? static_cast<components::ql::ql_param_statement_t*>(statement)->take_parameters()
                              : components::ql::storage_parameters{};
        return {logic_plan, parameters};
    }

    void memory_storage_t::wal_success(components::session::session_id_t& session, services::wal::id_t wal_id) {
        trace(log_, "memory_storage_t::wal_success session : {}, wal id: {}", session.data(), wal_id);
        actor_zeta::send(manager_disk_, address(), disk::handler_id(disk::route::flush), session, wal_id);

        if (!sessions_.contains(session)) {
            return;
        }

        auto session_obj = sessions_.at(session);
        const bool is_from_wal = session_obj.address().get() == manager_wal_.get();
        if (is_from_wal) {
            return;
        }

        trace(log_, "memory_storage_t::wal_success remove session : {}, wal id: {}", session.data(), wal_id);
        auto result = result_storage_[session];
        actor_zeta::send(session_obj.address(),
                         address(),
                         memory_storage::handler_id(memory_storage::route::execute_ql_finish),
                         session,
                         result);
        sessions_.erase(session);
        result_storage_.erase(session);
    }

    void memory_storage_t::load_from_wal_result(components::session::session_id_t& session,
                                                std::vector<services::wal::record_t>& in_records) {
        // TODO think what to do with records
        records_ = std::move(in_records);
        load_count_answers_ = records_.size();
        trace(log_,
              "memory_storage_t::load_from_wal_result, session: {}, count commands: {}",
              session.data(),
              load_count_answers_);
        if (load_count_answers_ == 0) {
            trace(log_, "memory_storage_t::load_from_wal_result - empty records_");
            actor_zeta::send(sessions_.at(session).address(), address(), core::handler_id(core::route::load_finish));
            sessions_.erase(session);
            return;
        }
        last_wal_id_ = records_[load_count_answers_ - 1].id;
        for (auto& record : records_) {
            switch (record.type) {
                case statement_type::create_database: {
                    assert(std::holds_alternative<create_database_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::create_database] variant "
                           "record.data holds the alternative create_database_t");
                    auto& data = std::get<create_database_t>(record.data);
                    components::session::session_id_t session_database;
                    execute_ql(session_database, &data, manager_wal_);
                    break;
                }
                case statement_type::drop_database: {
                    assert(std::holds_alternative<drop_database_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::drop_database] variant "
                           "record.data holds the alternative drop_database_t");
                    auto& data = std::get<drop_database_t>(record.data);
                    components::session::session_id_t session_database;
                    execute_ql(session_database, &data, manager_wal_);
                    break;
                }
                case statement_type::create_collection: {
                    assert(
                        std::holds_alternative<create_collection_t>(record.data) &&
                        "[memory_storage_t::load_from_wal_result]: [ case: statement_type::create_collection] variant "
                        "record.data holds the alternative create_collection_t");
                    auto& data = std::get<create_collection_t>(record.data);
                    components::session::session_id_t session_collection;
                    execute_ql(session_collection, &data, manager_wal_);
                    break;
                }
                case statement_type::drop_collection: {
                    assert(std::holds_alternative<drop_collection_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::drop_collection] variant "
                           "record.data holds the alternative drop_collection_t");
                    auto& data = std::get<drop_collection_t>(record.data);
                    components::session::session_id_t session_collection;
                    execute_ql(session_collection, &data, manager_wal_);
                    break;
                }
                case statement_type::insert_one: {
                    trace(log_, "memory_storage_t::load_from_wal_result: insert_one {}", session.data());
                    assert(std::holds_alternative<insert_one_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::insert_one] variant "
                           "record.data holds the alternative insert_one_t");
                    auto& ql = std::get<insert_one_t>(record.data);
                    components::session::session_id_t session_insert;
                    execute_ql(session_insert, &ql, manager_wal_);
                    break;
                }
                case statement_type::insert_many: {
                    trace(log_, "memory_storage_t::load_from_wal_result: insert_many {}", session.data());
                    assert(std::holds_alternative<insert_many_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::insert_many] variant "
                           "record.data holds the alternative insert_many_t");
                    auto& ql = std::get<insert_many_t>(record.data);
                    components::session::session_id_t session_insert;
                    execute_ql(session_insert, &ql, manager_wal_);
                    break;
                }
                case statement_type::delete_one: {
                    assert(std::holds_alternative<delete_one_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::delete_one] variant "
                           "record.data holds the alternative delete_one_t");
                    auto& ql = std::get<delete_one_t>(record.data);
                    components::session::session_id_t session_delete;
                    execute_ql(session_delete, &ql, manager_wal_);
                    break;
                }
                case statement_type::delete_many: {
                    assert(std::holds_alternative<delete_many_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::delete_many] variant "
                           "record.data holds the alternative delete_many_t");
                    auto& ql = std::get<delete_many_t>(record.data);
                    components::session::session_id_t session_delete;
                    execute_ql(session_delete, &ql, manager_wal_);
                    break;
                }
                case statement_type::update_one: {
                    trace(log_, "memory_storage_t::load_from_wal_result: update_one {}", session.data());
                    assert(std::holds_alternative<update_one_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::update_one] variant "
                           "record.data holds the alternative update_one_t");
                    auto& ql = std::get<update_one_t>(record.data);
                    components::session::session_id_t session_update;
                    execute_ql(session_update, &ql, manager_wal_);
                    break;
                }
                case statement_type::update_many: {
                    assert(std::holds_alternative<update_many_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::update_many] variant "
                           "record.data holds the alternative update_many_t");
                    auto& ql = std::get<update_many_t>(record.data);
                    components::session::session_id_t session_update;
                    execute_ql(session_update, &ql, manager_wal_);
                    break;
                }
                case statement_type::create_index: {
                    assert(std::holds_alternative<create_index_t>(record.data) &&
                           "[memory_storage_t::load_from_wal_result]: [ case: statement_type::create_index] variant "
                           "record.data holds the alternative create_index_t");
                    auto& ql = std::get<create_index_t>(record.data);
                    components::session::session_id_t session_create_index;
                    execute_ql(session_create_index, &ql, manager_wal_);
                    break;
                }
                // TODO no drop index?
                default:
                    break;
            }
        }
    }

    void memory_storage_t::load_from_disk_finished(components::session::session_id_t& session) {
        trace(log_, "memory_storage_t:load_from_disk_finished: {}", session.data());
        actor_zeta::send(manager_disk_, address(), disk::handler_id(disk::route::load_indexes), session);
        actor_zeta::send(manager_wal_, address(), wal::handler_id(wal::route::load), session, load_result_.wal_id());
        load_result_.clear();
        load_buffer_.reset();
    }

    // TODO separate change logic and condition check
    bool memory_storage_t::load_from_wal_in_progress(components::session::session_id_t& session) {
        trace(log_, "memory_storage_t::load_from_wal_in_progress: session: {}", session.data());
        auto sender = sessions_.at(session).address();
        if (sender.get() == manager_wal_.get()) {
            if (--load_count_answers_ == 0) {
                actor_zeta::send(sessions_.at(load_session_).address(),
                                 address(),
                                 core::handler_id(core::route::load_finish));
                sessions_.erase(load_session_);
            }
            return true;
        }
        return false;
    }

    bool memory_storage_t::is_exists_database_(const database_name_t& name) const {
        return databases_.find(name) != databases_.end();
    }

    bool memory_storage_t::is_exists_collection_(const collection_full_name_t& name) const {
        return collections_.contains(name);
    }

    bool memory_storage_t::check_database_(components::session::session_id_t& session, const database_name_t& name) {
        if (!is_exists_database_(name)) {
            execute_ql_finish(
                session,
                make_cursor(default_resource(), error_code_t::database_not_exists, "database not exists"));
            return false;
        }
        return true;
    }

    bool memory_storage_t::check_collection_(components::session::session_id_t& session,
                                             const collection_full_name_t& name) {
        if (check_database_(session, name.database)) {
            if (!is_exists_collection_(name)) {
                execute_ql_finish(
                    session,
                    make_cursor(default_resource(), error_code_t::collection_not_exists, "collection not exists"));
                return false;
            }
            return true;
        }
        return false;
    }

    void memory_storage_t::create_database_(components::session::session_id_t& session,
                                            components::logical_plan::node_ptr logical_plan) {
        trace(log_, "memory_storage_t:create_database: {}", logical_plan->database_name());
        if (!is_exists_database_(logical_plan->database_name())) {
            databases_.insert(logical_plan->database_name());
            execute_ql_finish(session, make_cursor(default_resource(), operation_status_t::success));
        }
    }

    void memory_storage_t::drop_database_(components::session::session_id_t& session,
                                          components::logical_plan::node_ptr logical_plan) {
        trace(log_, "memory_storage_t:drop_database {}", logical_plan->database_name());
        if (check_database_(session, logical_plan->database_name())) {
            databases_.erase(logical_plan->database_name());
            execute_ql_finish(session, make_cursor(default_resource(), operation_status_t::success));
        }
    }

    void memory_storage_t::create_collection_(components::session::session_id_t& session,
                                              components::logical_plan::node_ptr logical_plan) {
        trace(log_, "memory_storage_t:create_collection {}", logical_plan->collection_full_name().to_string());
        if (check_database_(session, logical_plan->database_name())) {
            if (!is_exists_collection_(logical_plan->collection_full_name())) {
                collections_.emplace(logical_plan->collection_full_name(),
                                     new collection::context_collection_t(std::pmr::get_default_resource(),
                                                                          logical_plan->collection_full_name(),
                                                                          manager_disk_,
                                                                          log_.clone()));
                execute_ql_finish(session, make_cursor(default_resource(), operation_status_t::success));
            }
        }
    }

    void memory_storage_t::drop_collection_(components::session::session_id_t& session,
                                            components::logical_plan::node_ptr logical_plan) {
        trace(log_, "memory_storage_t:drop_collection {}", logical_plan->collection_full_name().to_string());
        if (check_database_(session, logical_plan->database_name())) {
            if (check_collection_(session, logical_plan->collection_full_name())) {
                execute_ql_finish(
                    session,
                    collections_.at(logical_plan->collection_full_name())->drop()
                        ? make_cursor(default_resource(), operation_status_t::success)
                        : make_cursor(default_resource(), error_code_t::other_error, "collection not dropped"));
                collections_.erase(logical_plan->collection_full_name());
            }
        }
    }

    void memory_storage_t::execute_plan_(components::session::session_id_t& session,
                                         components::logical_plan::node_ptr logical_plan,
                                         components::ql::storage_parameters parameters) {
        trace(log_,
              "memory_storage_t:execute_plan_: collection: {}, sesion: {}",
              logical_plan->collection_full_name().to_string(),
              session.data());
        auto dependency_tree_collections_names = logical_plan->collection_dependencies();
        context_storage_t collections_context_storage;
        while (!dependency_tree_collections_names.empty()) {
            collection_full_name_t name =
                dependency_tree_collections_names.extract(dependency_tree_collections_names.begin()).value();
            if (!check_collection_(session, name)) {
                return;
            }
            collections_context_storage.emplace(std::move(name), collections_.at(name).get());
        }
        actor_zeta::send(executor_address_,
                         address(),
                         collection::handler_id(collection::route::execute_plan),
                         session,
                         logical_plan,
                         parameters,
                         std::move(collections_context_storage));
    }

} // namespace services
