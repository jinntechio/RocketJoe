#include "agent_disk.hpp"
#include "manager_disk.hpp"
#include "result.hpp"
#include "route.hpp"

namespace services::disk {

    agent_disk_t::agent_disk_t(base_manager_disk_t* manager, const path_t& path_db, const name_t& name, log_t& log)
        : actor_zeta::basic_async_actor(manager, name)
        , log_(log.clone())
        , disk_(path_db) {
        trace(log_, "agent_disk::create");
        add_handler(handler_id(route::load), &agent_disk_t::load);
        add_handler(handler_id(route::append_database), &agent_disk_t::append_database);
        add_handler(handler_id(route::remove_database), &agent_disk_t::remove_database);
        add_handler(handler_id(route::append_collection), &agent_disk_t::append_collection);
        add_handler(handler_id(route::remove_collection), &agent_disk_t::remove_collection);
        add_handler(handler_id(route::write_documents), &agent_disk_t::write_documents);
        add_handler(handler_id(route::remove_documents), &agent_disk_t::remove_documents);
        add_handler(handler_id(route::fix_wal_id), &agent_disk_t::fix_wal_id);
    }

    agent_disk_t::~agent_disk_t() { trace(log_, "delete agent_disk_t"); }

    auto agent_disk_t::load(session_id_t& session, actor_zeta::address_t dispatcher) -> void {
        trace(log_, "agent_disk::load , session : {} , disk databases: {}", session.data(), disk_.databases().size());
        result_load_t result(disk_.databases(), disk_.wal_id());
        for (auto& database : *result) {
            database.set_collection(disk_.collections(database.name));
            for (auto& collection : database.collections) {
                auto id_documents = disk_.load_list_documents(database.name, collection.name);
                for (const auto& id : id_documents) {
                    collection.documents.push_back(disk_.load_document(id));
                }
            }
        }
        actor_zeta::send(dispatcher, address(), handler_id(route::load_finish), session, std::move(result));
    }

    auto agent_disk_t::append_database(const command_t& command) -> void {
        auto& cmd = command.get<command_append_database_t>();
        trace(log_, "agent_disk::append_database , database : {}", cmd.database);
        disk_.append_database(cmd.database);
    }

    auto agent_disk_t::remove_database(const command_t& command) -> void {
        auto& cmd = command.get<command_remove_database_t>();
        trace(log_, "agent_disk::remove_database , database : {}", cmd.database);
        disk_.remove_database(cmd.database);
    }

    auto agent_disk_t::append_collection(const command_t& command) -> void {
        auto& cmd = command.get<command_append_collection_t>();
        trace(log_, "agent_disk::append_collection , database : {} , collection : {}", cmd.database, cmd.collection);
        disk_.append_collection(cmd.database, cmd.collection);
    }

    auto agent_disk_t::remove_collection(const command_t& command) -> void {
        auto& cmd = command.get<command_remove_collection_t>();
        trace(log_, "agent_disk::remove_collection , database : {} , collection : {}", cmd.database, cmd.collection);
        disk_.remove_collection(cmd.database, cmd.collection);
    }

    auto agent_disk_t::write_documents(const command_t& command) -> void {
        auto& write_command = command.get<command_write_documents_t>();
        trace(log_,
              "agent_disk::write_documents , database : {} , collection : {} , {} documents",
              write_command.database,
              write_command.collection,
              write_command.documents.size());
        for (const auto& document : write_command.documents) {
            auto id = components::document::get_document_id(document);
            if (!id.is_null()) {
                disk_.save_document(write_command.database, write_command.collection, id, document);
            }
        }
    }

    auto agent_disk_t::remove_documents(const command_t& command) -> void {
        auto& remove_command = command.get<command_remove_documents_t>();
        trace(log_,
              "agent_disk::remove_documents , database : {} , collection : {} , {} documents",
              remove_command.database,
              remove_command.collection,
              remove_command.documents.size());
        for (const auto& id : remove_command.documents) {
            disk_.remove_document(remove_command.database, remove_command.collection, id);
        }
    }

    auto agent_disk_t::fix_wal_id(wal::id_t wal_id) -> void {
        trace(log_, "agent_disk::fix_wal_id : {}", wal_id);
        disk_.fix_wal_id(wal_id);
    }

} //namespace services::disk