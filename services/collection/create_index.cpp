#include "executor.hpp"

#include <components/index/disk/route.hpp>
#include <components/index/single_field_index.hpp>
#include <components/ql/index.hpp>
#include <services/disk/index_disk.hpp>

#include <services/collection/planner/create_plan.hpp>
#include <services/disk/route.hpp>
#include <services/memory_storage/memory_storage.hpp>

using components::ql::create_index_t;
using components::ql::index_type;

using components::index::make_index;
using components::index::single_field_index_t;

using namespace components::cursor;
using namespace core::pmr;

namespace services::collection::executor {

    std::string keys_index(const components::ql::keys_base_storage_t& keys) {
        std::string result;
        for (const auto& key : keys) {
            if (!result.empty()) {
                result += ",";
            }
            result += key.as_string();
        }
        return result;
    }

    void executor_t::create_index_finish(const session_id_t& session,
                                         const std::string& name,
                                         const actor_zeta::address_t& index_address,
                                         context_collection_t* collection) {
        trace(log_, "executor::create_index_finish");
        auto& create_index = sessions::find(collection->sessions(), session, name).get<sessions::create_index_t>();
        components::index::set_disk_agent(collection->index_engine(), create_index.id_index, index_address);
        components::index::insert(collection->index_engine(), create_index.id_index, collection->storage());
        if (!create_index.is_pending) {
            actor_zeta::send(create_index.client,
                             address(),
                             handler_id(route::execute_plan_finish),
                             session,
                             make_cursor(default_resource(), operation_status_t::success));
        }
        sessions::remove(collection->sessions(), session, name);
    }

    void executor_t::create_index_finish_index_exist(const session_id_t& session,
                                                     const std::string& name,
                                                     context_collection_t* collection) {
        trace(log_, "executor::create_index_finish_index_exist");
        auto& create_index = sessions::find(collection->sessions(), session, name).get<sessions::create_index_t>();
        if (!create_index.is_pending) {
            actor_zeta::send(create_index.client,
                             address(),
                             handler_id(route::execute_plan_finish),
                             session,
                             make_cursor(default_resource(),
                                         error_code_t::index_create_fail,
                                         "index with name : " + name + " exist"));
        }
        sessions::remove(collection->sessions(), session, name);
    }

    void executor_t::index_modify_finish(const session_id_t& session, context_collection_t* collection) {
        trace(log_, "executor::index_modify_finish");
        sessions::remove(collection->sessions(), session);
    }

    void executor_t::index_find_finish(const session_id_t& session,
                                       const std::pmr::vector<document_id_t>& result,
                                       context_collection_t* collection) {
        trace(log_, "executor::index_find_result: {}", result.size());
        components::index::sync_index_from_disk(collection->index_engine(),
                                                current_message()->sender(),
                                                result,
                                                collection->storage());
        auto& suspend_plan = sessions::find(collection->sessions(), session).get<sessions::suspend_plan_t>();
        auto res = collection->cursor_storage().emplace(
            session,
            std::make_unique<components::cursor::sub_cursor_t>(collection->resource(), collection->name()));
        suspend_plan.plan->on_execute(&suspend_plan.pipeline_context);
        if (suspend_plan.plan->is_executed()) {
            if (suspend_plan.plan->output()) {
                for (const auto& document : suspend_plan.plan->output()->documents()) {
                    res.first->second->append(document_view_t(document));
                }
            }
        }
        sessions::remove(collection->sessions(), session);
        auto cursor = make_cursor(default_resource());
        cursor->push(res.first->second.get());
        execute_sub_plan_finish_(session, cursor);
    }

} // namespace services::collection::executor
