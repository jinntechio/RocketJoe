#include "executor.hpp"

#include <components/index/disk/route.hpp>
#include <core/system_command.hpp>
#include <services/collection/create_index_utils.hpp>
#include <services/collection/operators/operator_delete.hpp>
#include <services/collection/operators/operator_insert.hpp>
#include <services/collection/operators/operator_update.hpp>
#include <services/collection/operators/scan/primary_key_scan.hpp>
#include <services/collection/planner/create_plan.hpp>
#include <services/disk/route.hpp>
#include <services/memory_storage/memory_storage.hpp>

using namespace components::cursor;

namespace services::collection::executor {

    plan_t::plan_t(std::stack<collection::operators::operator_ptr>&& sub_plans,
                   components::ql::storage_parameters parameters,
                   services::context_storage_t&& context_storage)
        : sub_plans(std::move(sub_plans))
        , parameters(parameters)
        , context_storage_(context_storage) {}

    executor_t::executor_t(services::memory_storage_t* memory_storage, std::pmr::memory_resource* resource, log_t&& log)
        : actor_zeta::basic_async_actor(memory_storage, "plan executor")
        , memory_storage_(memory_storage->address())
        , resource_(resource)
        , plans_(resource_)
        , log_(log) {
        add_handler(handler_id(route::execute_plan), &executor_t::execute_plan);
        add_handler(handler_id(index::route::success_create), &executor_t::create_index_finish);
        add_handler(handler_id(index::route::error), &executor_t::create_index_finish_index_exist);
        add_handler(handler_id(index::route::success), &executor_t::index_modify_finish);
        add_handler(handler_id(index::route::success_find), &executor_t::index_find_finish);
    }

    void executor_t::execute_plan(const components::session::session_id_t& session,
                                  components::logical_plan::node_ptr logical_plan,
                                  components::ql::storage_parameters parameters,
                                  services::context_storage_t&& context_storage) {
        trace(log_, "executor::execute_plan, session: {}", session.data());
        auto plan = collection::planner::create_plan(context_storage, logical_plan, components::ql::limit_t::unlimit());
        if (!plan) {
            actor_zeta::send(memory_storage_,
                             address(),
                             handler_id(route::execute_plan_finish),
                             session,
                             make_cursor(std::pmr::get_default_resource(),
                                         error_code_t::create_phisical_plan_error,
                                         "invalid query plan"));
            return;
        }
        plan->set_as_root();
        traverse_plan_(session, std::move(plan), std::move(parameters), std::move(context_storage));
    }

    void executor_t::traverse_plan_(const components::session::session_id_t& session,
                                    collection::operators::operator_ptr&& plan,
                                    components::ql::storage_parameters&& parameters,
                                    services::context_storage_t&& context_storage) {
        std::stack<collection::operators::operator_ptr> look_up;
        std::stack<collection::operators::operator_ptr> sub_plans;
        look_up.push(plan);
        while (!look_up.empty()) {
            auto check_op = look_up.top();
            while (check_op->right() == nullptr) {
                check_op = check_op->left();
                if (check_op == nullptr) {
                    break;
                }
            }
            sub_plans.push(look_up.top());
            look_up.pop();
            if (check_op != nullptr) {
                look_up.push(check_op->right());
                look_up.push(check_op->left());
            }
        }

        trace(log_, "executor::subplans count {}", sub_plans.size());
        // start execution chain by sending first avaliable sub_plan
        auto current_plan =
            (plans_.emplace(session, plan_t{std::move(sub_plans), parameters, std::move(context_storage)}))
                .first->second.sub_plans.top();
        execute_sub_plan_(session, current_plan, parameters);
    }

    void executor_t::execute_sub_plan_(const components::session::session_id_t& session,
                                       collection::operators::operator_ptr plan,
                                       components::ql::storage_parameters parameters) {
        trace(log_, "executor::execute_sub_plan, session: {}", session.data());
        auto collection = plan->context();
        if (collection->dropped()) {
            execute_sub_plan_finish_(
                session,
                make_cursor(collection->resource(), error_code_t::collection_dropped, "collection dropped"));
            return;
        }
        if (!plan) {
            execute_sub_plan_finish_(
                session,
                make_cursor(collection->resource(), error_code_t::create_phisical_plan_error, "invalid query plan"));
            return;
        }
        components::pipeline::context_t pipeline_context{session, address(), memory_storage_, parameters};
        plan->on_execute(&pipeline_context);
        // TODO figure is it possible to use for pending indexes
        if (!plan->is_executed()) {
            sessions::make_session(
                collection->sessions(),
                session,
                sessions::suspend_plan_t{memory_storage_, std::move(plan), std::move(pipeline_context)});
            return;
        }

        switch (plan->type()) {
            case operators::operator_type::insert: {
                insert_document_impl(session, collection, std::move(plan));
                return;
            }
            case operators::operator_type::remove: {
                delete_document_impl(session, collection, std::move(plan));
                return;
            }
            case operators::operator_type::update: {
                update_document_impl(session, collection, std::move(plan));
                return;
            }
            case operators::operator_type::join:
            case operators::operator_type::aggregate: {
                aggregate_document_impl(session, collection, std::move(plan));
                return;
            }
            case operators::operator_type::add_index:
            case operators::operator_type::drop_index: {
                // nothing to do
                return;
            }
            default: {
                execute_sub_plan_finish_(session, make_cursor(collection->resource(), operation_status_t::success));
                return;
            }
        }
    }

    void executor_t::execute_sub_plan_finish_(const components::session::session_id_t& session, cursor_t_ptr result) {
        if (result->is_error() || !plans_.contains(session)) {
            execute_plan_finish_(session, std::move(result));
        }
        auto& plan = plans_.at(session);
        if (plan.sub_plans.size() == 1) {
            execute_plan_finish_(session, std::move(result));
        } else {
            assert(!plan.sub_plans.empty() && "executor_t:execute_sub_plan_finish_: sub plans execution failed");
            plan.sub_plans.pop();
            execute_sub_plan_(session, plan.sub_plans.top(), plan.parameters);
        }
    }

    void executor_t::execute_plan_finish_(const components::session::session_id_t& session, cursor_t_ptr&& cursor) {
        trace(log_, "executor::execute_plan_finish, success: {}", cursor->is_success());
        actor_zeta::send(memory_storage_,
                         address(),
                         handler_id(route::execute_plan_finish),
                         session,
                         std::move(cursor));
        plans_.erase(session);
    }

    void executor_t::aggregate_document_impl(const components::session::session_id_t& session,
                                             context_collection_t* collection,
                                             operators::operator_ptr plan) {
        if (plan->type() == operators::operator_type::aggregate) {
            trace(log_, "executor::execute_plan : operators::operator_type::agreggate");
        } else {
            trace(log_, "executor::execute_plan : operators::operator_type::join");
        }
        if (plan->is_root()) {
            auto cursor = collection->cursor_storage().emplace(
                session,
                std::make_unique<components::cursor::sub_cursor_t>(collection->resource(), collection->name()));
            if (plan->output()) {
                for (const auto& document : plan->output()->documents()) {
                    cursor.first->second->append(document_view_t(document));
                }
            }
            if (cursor.first->second.get()->size() == 0) {
                execute_sub_plan_finish_(session, make_cursor(collection->resource(), operation_status_t::failure));
            } else {
                auto result = make_cursor(collection->resource(), operation_status_t::success);
                result->push(cursor.first->second.get());
                execute_sub_plan_finish_(session, std::move(result));
            }
        } else {
            execute_sub_plan_finish_(session, make_cursor(collection->resource(), operation_status_t::success));
        }
    }

    void executor_t::update_document_impl(const components::session::session_id_t& session,
                                          context_collection_t* collection,
                                          operators::operator_ptr plan) {
        trace(log_, "executor::execute_plan : operators::operator_type::update");

        if (plan->output()) {
            auto new_id = components::document::get_document_id(plan->output()->documents().front());
            std::pmr::vector<document_id_t> documents{collection->resource()};
            documents.emplace_back(new_id);
            actor_zeta::send(collection->disk(),
                             address(),
                             disk::handler_id(disk::route::remove_documents),
                             session,
                             std::string(collection->name().database),
                             std::string(collection->name().collection),
                             documents);
            auto cursor(new cursor_t(collection->resource()));
            auto* sub_cursor = new sub_cursor_t(collection->resource(), collection->name());
            for (const auto& id : documents) {
                sub_cursor->append(document_view_t(collection->storage().at(id)));
            }
            cursor->push(sub_cursor);
            execute_sub_plan_finish_(session, cursor);
        } else {
            if (plan->modified()) {
                auto cursor(new cursor_t(collection->resource()));
                auto* sub_cursor =
                    new sub_cursor_t(plan->modified()->documents().get_allocator().resource(), collection->name());
                for (const auto& id : plan->modified()->documents()) {
                    sub_cursor->append(document_view_t(collection->storage().at(id)));
                }
                cursor->push(sub_cursor);
                actor_zeta::send(collection->disk(),
                                 address(),
                                 disk::handler_id(disk::route::remove_documents),
                                 session,
                                 std::string(collection->name().database),
                                 std::string(collection->name().collection),
                                 plan->modified()->documents());
                execute_sub_plan_finish_(session, cursor);
            } else {
                auto cursor(new cursor_t(collection->resource()));
                auto* sub_cursor = new sub_cursor_t(collection->resource(), collection->name());
                cursor->push(sub_cursor);
                actor_zeta::send(collection->disk(),
                                 address(),
                                 disk::handler_id(disk::route::remove_documents),
                                 session,
                                 std::string(collection->name().database),
                                 std::string(collection->name().collection),
                                 std::pmr::vector<document_id_t>{collection->resource()});
                execute_sub_plan_finish_(session, cursor);
            }
        }
    }

    void executor_t::insert_document_impl(const components::session::session_id_t& session,
                                          context_collection_t* collection,
                                          operators::operator_ptr plan) {
        trace(log_, "executor::execute_plan : operators::operator_type::insert");
        actor_zeta::send(collection->disk(),
                         address(),
                         disk::handler_id(disk::route::write_documents),
                         session,
                         std::string(collection->name().database),
                         std::string(collection->name().collection),
                         plan->output() ? plan->output()->documents()
                                        : std::pmr::vector<document_ptr>{collection->resource()});
        trace(log_, "executor::execute_plan : operators::operator_type::insert sent to disk");
        if (!collection->pending_indexes().empty()) {
            process_pending_indexes(collection);
            trace(log_, "executor::execute_plan : operators::operator_type::insert indexes processed");
        }

        auto cursor = make_cursor(collection->resource());
        auto* sub_cursor = new sub_cursor_t(collection->resource(), collection->name());
        if (plan->modified()) {
            for (const auto& id : plan->modified()->documents()) {
                sub_cursor->append(document_view_t(collection->storage().at(id)));
            }
        } else {
            for (const auto& doc : collection->storage()) {
                sub_cursor->append(document_view_t(doc.second));
            }
        }
        cursor->push(sub_cursor);
        execute_sub_plan_finish_(session, cursor);
    }

    void executor_t::delete_document_impl(const components::session::session_id_t& session,
                                          context_collection_t* collection,
                                          operators::operator_ptr plan) {
        trace(log_, "executor::execute_plan : operators::operator_type::remove");

        auto modified =
            plan->modified() ? plan->modified()->documents() : std::pmr::vector<document_id_t>{collection->resource()};
        actor_zeta::send(collection->disk(),
                         address(),
                         disk::handler_id(disk::route::remove_documents),
                         session,
                         std::string(collection->name().database),
                         std::string(collection->name().collection),
                         modified);
        auto* sub_cursor =
            new sub_cursor_t(plan->modified()->documents().get_allocator().resource(), collection->name());
        for (const auto& id : plan->modified()->documents()) {
            sub_cursor->append(document_view_t(nullptr));
        }
        auto cursor = make_cursor(collection->resource());
        cursor->push(sub_cursor);
        execute_sub_plan_finish_(session, cursor);
    }

} // namespace services::collection::executor