#include "manager_disk.hpp"
#include "result.hpp"
#include "route.hpp"
#include <components/index/disk/route.hpp>
#include <core/system_command.hpp>
#include <services/collection/collection.hpp>
#include <services/collection/route.hpp>
#include <services/memory_storage/route.hpp>

namespace services::disk {

    using components::document::document_id_t;

    namespace {
        std::vector<std::unique_ptr<components::ql::create_index_t>>
        make_unique(std::vector<components::ql::create_index_t> indexes) {
            std::vector<std::unique_ptr<components::ql::create_index_t>> result;
            result.reserve(indexes.size());

            for (auto&& index : indexes) {
                result.push_back(std::make_unique<components::ql::create_index_t>(std::move(index)));
            }
            return result;
        }
    } // namespace

    base_manager_disk_t::base_manager_disk_t(actor_zeta::detail::pmr::memory_resource* mr,
                                             actor_zeta::scheduler_raw scheduler)
        : actor_zeta::cooperative_supervisor<base_manager_disk_t>(mr, "manager_disk")
        , e_(scheduler) {}

    auto base_manager_disk_t::scheduler_impl() noexcept -> actor_zeta::scheduler_abstract_t* { return e_; }

    auto base_manager_disk_t::enqueue_impl(actor_zeta::message_ptr msg, actor_zeta::execution_unit*) -> void {
        set_current_message(std::move(msg));
        execute(this, current_message());
    }

    manager_disk_t::manager_disk_t(actor_zeta::detail::pmr::memory_resource* mr,
                                   actor_zeta::scheduler_raw scheduler,
                                   configuration::config_disk config,
                                   log_t& log)
        : base_manager_disk_t(mr, scheduler)
        , log_(log.clone())
        , config_(std::move(config))
        , metafile_indexes_(nullptr)
        , removed_indexes_(mr) {
        trace(log_, "manager_disk start");
        add_handler(core::handler_id(core::route::sync), &manager_disk_t::sync);
        add_handler(handler_id(route::create_agent), &manager_disk_t::create_agent);
        add_handler(handler_id(route::load), &manager_disk_t::load);
        add_handler(handler_id(route::load_indexes), &manager_disk_t::load_indexes);
        add_handler(handler_id(route::append_database), &manager_disk_t::append_database);
        add_handler(handler_id(route::remove_database), &manager_disk_t::remove_database);
        add_handler(handler_id(route::append_collection), &manager_disk_t::append_collection);
        add_handler(handler_id(route::remove_collection), &manager_disk_t::remove_collection);
        add_handler(handler_id(route::write_documents), &manager_disk_t::write_documents);
        add_handler(handler_id(route::remove_documents), &manager_disk_t::remove_documents);
        add_handler(handler_id(route::flush), &manager_disk_t::flush);
        add_handler(handler_id(index::route::create), &manager_disk_t::create_index_agent);
        add_handler(handler_id(index::route::drop), &manager_disk_t::drop_index_agent);
        add_handler(handler_id(index::route::success), &manager_disk_t::drop_index_agent_success);
        if (!config_.path.empty()) {
            if (!std::filesystem::is_directory(config_.path / "indexes")) {
                std::filesystem::create_directories(config_.path / "indexes");
            }
            metafile_indexes_ = std::make_unique<core::file::file_t>(config_.path / "indexes/METADATA");
        }
        trace(log_, "manager_disk finish");
    }

    manager_disk_t::~manager_disk_t() { trace(log_, "delete manager_disk_t"); }

    void manager_disk_t::create_agent() {
        auto name_agent = "agent_disk_" + std::to_string(agents_.size() + 1);
        trace(log_, "manager_disk create_agent : {}", name_agent);
        auto address =
            spawn_actor<agent_disk_t>([this](agent_disk_t* ptr) { agents_.emplace_back(agent_disk_ptr(ptr)); },
                                      config_.path,
                                      name_agent,
                                      log_);
    }

    auto manager_disk_t::load(session_id_t& session) -> void {
        trace(log_, "manager_disk_t::load , session : {}", session.data());
        actor_zeta::send(agent(), address(), handler_id(route::load), session, current_message()->sender());
    }

    auto manager_disk_t::load_indexes(session_id_t& session) -> void {
        trace(log_, "manager_disk_t::load_indexes , session : {}", session.data());
        load_session_ = session;
        load_indexes_(session, current_message()->sender());
    }

    auto manager_disk_t::append_database(session_id_t& session, const database_name_t& database) -> void {
        trace(log_, "manager_disk_t::append_database , session : {} , database : {}", session.data(), database);
        command_append_database_t command{database};
        append_command(commands_, session, command_t(command));
    }

    auto manager_disk_t::remove_database(session_id_t& session, const database_name_t& database) -> void {
        trace(log_, "manager_disk_t::remove_database , session : {} , database : {}", session.data(), database);
        command_remove_database_t command{database};
        append_command(commands_, session, command_t(command));
    }

    auto manager_disk_t::append_collection(session_id_t& session,
                                           const database_name_t& database,
                                           const collection_name_t& collection) -> void {
        trace(log_,
              "manager_disk_t::append_collection , session : {} , database : {} , collection : {}",
              session.data(),
              database,
              collection);
        command_append_collection_t command{database, collection};
        append_command(commands_, session, command_t(command));
    }

    auto manager_disk_t::remove_collection(session_id_t& session,
                                           const database_name_t& database,
                                           const collection_name_t& collection) -> void {
        trace(log_,
              "manager_disk_t::remove_collection , session : {} , database : {} , collection : {}",
              session.data(),
              database,
              collection);
        command_remove_collection_t command{database, collection};
        append_command(commands_, session, command_t(command));
    }

    auto manager_disk_t::write_documents(session_id_t& session,
                                         const database_name_t& database,
                                         const collection_name_t& collection,
                                         const std::pmr::vector<document_ptr>& documents) -> void {
        trace(log_,
              "manager_disk_t::write_documents , session : {} , database : {} , collection : {}, documents count: {}",
              session.data(),
              database,
              collection,
              documents.size());
        command_write_documents_t command{database, collection, documents};
        append_command(commands_, session, command_t(command));
    }

    auto manager_disk_t::remove_documents(session_id_t& session,
                                          const database_name_t& database,
                                          const collection_name_t& collection,
                                          const std::pmr::vector<document_id_t>& documents) -> void {
        trace(log_,
              "manager_disk_t::remove_documents , session : {} , database : {} , collection : {}",
              session.data(),
              database,
              collection);
        command_remove_documents_t command{database, collection, documents};
        append_command(commands_, session, command_t(command));
    }

    auto manager_disk_t::flush(session_id_t& session, wal::id_t wal_id) -> void {
        trace(log_, "manager_disk_t::flush , session : {} , wal_id : {}", session.data(), wal_id);
        auto it = commands_.find(session);
        if (it != commands_.end()) {
            for (const auto& command : commands_.at(session)) {
                if (command.name() == handler_id(route::remove_collection)) {
                    const auto& drop_collection = command.get<command_remove_collection_t>();
                    std::vector<index_agent_disk_t*> indexes;
                    for (const auto& index : index_agents_) {
                        if (index.second->collection_name() == drop_collection.collection) {
                            indexes.push_back(index.second.get());
                        }
                    }
                    if (indexes.empty()) {
                        actor_zeta::send(agent(), address(), command.name(), command);
                    } else {
                        removed_indexes_.emplace(session,
                                                 removed_index_t{indexes.size(), command, current_message()->sender()});
                        for (auto* index : indexes) {
                            actor_zeta::send(index->address(),
                                             address(),
                                             index::handler_id(index::route::drop),
                                             session);
                        }
                    }
                } else {
                    actor_zeta::send(agent(), address(), command.name(), command);
                }
            }
            commands_.erase(session);
        }
        actor_zeta::send(agent(), address(), handler_id(route::fix_wal_id), wal_id);
    }

    void manager_disk_t::create_index_agent(session_id_t& session,
                                            const components::ql::create_index_t& index,
                                            services::collection::context_collection_t* collection) {
        auto name = index.name();
        trace(log_, "manager_disk: create_index_agent : {}", name);
        if (index_agents_.contains(name) && !index_agents_.at(name)->is_dropped()) {
            error(log_, "manager_disk: index {} already exists", name);
            actor_zeta::send(current_message()->sender(),
                             address(),
                             index::handler_id(index::route::error),
                             session,
                             name,
                             collection);
        } else {
            trace(log_, "manager_disk: create_index_agent : creating index: {}", name);
            index_agents_.erase(name);
            auto address_agent = spawn_actor<index_agent_disk_t>(
                [&](index_agent_disk_t* ptr) { index_agents_.insert_or_assign(name, index_agent_disk_ptr(ptr)); },
                resource(),
                config_.path,
                collection,
                name,
                index.index_compare_,
                log_);
            if (session.data() != load_session_.data()) {
                write_index_(index);
            }
            actor_zeta::send(current_message()->sender(),
                             address(),
                             index::handler_id(index::route::success_create),
                             session,
                             name,
                             address_agent,
                             collection);
        }
        trace(log_, "manager_disk: create_index_agent finished: {}", name);
    }

    void manager_disk_t::drop_index_agent(session_id_t& session,
                                          const index_name_t& index_name,
                                          services::collection::context_collection_t* collection) {
        if (index_agents_.contains(index_name)) {
            trace(log_, "manager_disk: drop_index_agent : {}", index_name);
            command_drop_index_t command{index_name, current_message()->sender()};
            append_command(commands_, session, command_t(command));
            actor_zeta::send(index_agents_.at(index_name)->address(),
                             address(),
                             index::handler_id(index::route::drop),
                             session,
                             collection);
            remove_index_(index_name);
        } else {
            error(log_, "manager_disk: index {} not exists", index_name);
            //actor_zeta::send(current_message()->sender(), address(), index::handler_id(index::route::error), session, collection);
        }
    }

    void manager_disk_t::drop_index_agent_success(session_id_t& session) {
        auto it = commands_.find(session);
        if (it != commands_.end()) {
            for (const auto& command : commands_.at(session)) {
                auto command_drop = command.get<command_drop_index_t>();
                trace(log_, "manager_disk: drop_index_agent : {} : success", command_drop.index_name);
                //actor_zeta::send(command_drop.address, address(), index::handler_id(index::route::success), session);
            }
            commands_.erase(session);
        } else {
            auto it_all_drop = removed_indexes_.find(session);
            if (it_all_drop != removed_indexes_.end()) {
                if (--it_all_drop->second.size == 0) {
                    actor_zeta::send(agent(),
                                     address(),
                                     it_all_drop->second.command.name(),
                                     it_all_drop->second.command);
                    const auto& drop_collection = it_all_drop->second.command.get<command_remove_collection_t>();
                    remove_all_indexes_from_collection_(drop_collection.collection);
                }
            }
        }
    }

    auto manager_disk_t::agent() -> actor_zeta::address_t { return agents_[0]->address(); }

    void manager_disk_t::write_index_(const components::ql::create_index_t& index) {
        if (metafile_indexes_) {
            msgpack::sbuffer buf;
            msgpack::pack(buf, index);
            auto size = buf.size();
            metafile_indexes_->append(reinterpret_cast<void*>(&size), sizeof(size));
            metafile_indexes_->append(buf.data(), buf.size());
        }
    }

    void manager_disk_t::load_indexes_([[maybe_unused]] session_id_t& session, const actor_zeta::address_t& storage) {
        auto indexes = make_unique(read_indexes_());
        metafile_indexes_->seek_eof();
        trace(log_, "manager_disk: load_indexes_ : size:{}", indexes.size());
        for (auto& index : indexes) {
            trace(log_, "manager_disk: load_indexes_ : {}", index->name()); // last one to be called
            // Require to separate sessions for load and create index
            // For each index create we need to generate unique session id.
            actor_zeta::send(storage,
                             address(),
                             memory_storage::handler_id(memory_storage::route::execute_ql),
                             session_id_t::generate_uid(),
                             index.release(),
                             storage);
            trace(log_, "manager_disk: load_indexes_ : sent to storage"); // is not called
        }
    }

    std::vector<components::ql::create_index_t>
    manager_disk_t::read_indexes_(const collection_name_t& collection) const {
        std::vector<components::ql::create_index_t> res;
        if (metafile_indexes_) {
            constexpr auto count_byte_by_size = sizeof(size_t);
            size_t size;
            __off64_t offset = 0;
            while (true) {
                auto size_str = metafile_indexes_->read(count_byte_by_size, offset);
                if (size_str.size() == count_byte_by_size) {
                    offset += count_byte_by_size;
                    std::memcpy(&size, size_str.data(), count_byte_by_size);
                    auto buf = metafile_indexes_->read(size, offset);
                    offset += __off64_t(size);
                    msgpack::unpacked msg;
                    msgpack::unpack(msg, buf.data(), buf.size());
                    auto index = msg.get().as<components::ql::create_index_t>();
                    if (collection.empty() || index.collection_ == collection) {
                        res.push_back(index);
                    }
                } else {
                    break;
                }
            }
        }
        return res;
    }

    std::vector<components::ql::create_index_t> manager_disk_t::read_indexes_() const { return read_indexes_(""); }

    void manager_disk_t::remove_index_(const index_name_t& index_name) {
        if (metafile_indexes_) {
            auto indexes = read_indexes_();
            indexes.erase(std::remove_if(indexes.begin(),
                                         indexes.end(),
                                         [&index_name](const components::ql::create_index_t& index) {
                                             return index.name() == index_name;
                                         }),
                          indexes.end());
            metafile_indexes_->clear();
            for (const auto& index : indexes) {
                write_index_(index);
            }
        }
    }

    void manager_disk_t::remove_all_indexes_from_collection_(const collection_name_t& collection) {
        if (metafile_indexes_) {
            auto indexes = read_indexes_();
            indexes.erase(std::remove_if(indexes.begin(),
                                         indexes.end(),
                                         [&collection](const components::ql::create_index_t& index) {
                                             return index.collection_ == collection;
                                         }),
                          indexes.end());
            metafile_indexes_->clear();
            for (const auto& index : indexes) {
                write_index_(index);
            }
        }
    }

    manager_disk_empty_t::manager_disk_empty_t(actor_zeta::detail::pmr::memory_resource* mr,
                                               actor_zeta::scheduler_raw scheduler,
                                               log_t& log)
        : base_manager_disk_t(mr, scheduler)
        , log_(log.clone()) {
        add_handler(core::handler_id(core::route::sync),
                    &manager_disk_empty_t::nothing<std::tuple<actor_zeta::address_t, actor_zeta::address_t>>);
        add_handler(handler_id(route::create_agent), &manager_disk_empty_t::nothing<>);
        add_handler(handler_id(route::load), &manager_disk_empty_t::load);
        add_handler(handler_id(route::append_database),
                    &manager_disk_empty_t::nothing<session_id_t&, const database_name_t&>);
        add_handler(handler_id(route::remove_database),
                    &manager_disk_empty_t::nothing<session_id_t&, const database_name_t&>);
        add_handler(handler_id(route::append_collection),
                    &manager_disk_empty_t::nothing<session_id_t&, const database_name_t&, const collection_name_t&>);
        add_handler(handler_id(route::remove_collection),
                    &manager_disk_empty_t::nothing<session_id_t&, const database_name_t&, const collection_name_t&>);
        add_handler(handler_id(route::write_documents),
                    &manager_disk_empty_t::nothing<session_id_t&,
                                                   const database_name_t&,
                                                   const collection_name_t&,
                                                   const std::vector<document_ptr>&>);
        add_handler(handler_id(route::remove_documents),
                    &manager_disk_empty_t::nothing<session_id_t&,
                                                   const database_name_t&,
                                                   const collection_name_t&,
                                                   const std::vector<document_id_t>&>);
        add_handler(handler_id(route::flush), &manager_disk_empty_t::nothing<session_id_t&, wal::id_t>);
        add_handler(handler_id(index::route::create), &manager_disk_empty_t::create_index_agent);
        add_handler(handler_id(index::route::drop), &manager_disk_empty_t::nothing<session_id_t&, const index_name_t&>);
    }

    auto manager_disk_empty_t::load(session_id_t& session) -> void {
        trace(log_, "manager_disk_empty_t::load");
        auto result = result_load_t::empty();
        actor_zeta::send(current_message()->sender(), address(), handler_id(route::load_finish), session, result);
    }

    void manager_disk_empty_t::create_index_agent(session_id_t& session,
                                                  const components::ql::create_index_t& index,
                                                  services::collection::context_collection_t* collection) {
        trace(log_, "manager_disk_empty_t::create_index_agent");
        auto name = index.name();
        actor_zeta::send(current_message()->sender(),
                         address(),
                         handler_id(index::route::success_create),
                         session,
                         name,
                         actor_zeta::address_t::empty_address(),
                         collection);
        trace(log_, "manager_disk_empty_t::create_index_agent finished");
    }

} //namespace services::disk