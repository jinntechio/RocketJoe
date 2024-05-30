#pragma once

#include "wrapper_dispatcher.hpp"
#include <components/configuration/configuration.hpp>
#include <components/log/log.hpp>
#include <core/excutor.hpp>

namespace services {

    namespace disk {
        class base_manager_disk_t;
        using manager_disk_ptr = std::unique_ptr<base_manager_disk_t>;
    } // namespace disk

    namespace wal {
        class base_manager_wal_replicate_t;
        using manager_wal_ptr = std::unique_ptr<base_manager_wal_replicate_t>;
    } // namespace wal

    class memory_storage_t;
    using memory_storage_ptr = std::unique_ptr<memory_storage_t>;

} // namespace services

namespace otterbrix {

    class base_otterbrix_t {
    public:
        base_otterbrix_t(base_otterbrix_t& other) = delete;
        void operator=(const base_otterbrix_t&) = delete;

        log_t& get_log();
        otterbrix::wrapper_dispatcher_t* dispatcher();
        ~base_otterbrix_t();

    protected:
        explicit base_otterbrix_t(const configuration::config& config);
        log_t log_;
        actor_zeta::scheduler_ptr scheduler_;
        actor_zeta::scheduler_ptr scheduler_dispather_;
        actor_zeta::detail::pmr::memory_resource* resource;
        services::disk::manager_disk_ptr manager_disk_;
        services::wal::manager_wal_ptr manager_wal_;
        services::memory_storage_ptr memory_storage_;
        std::unique_ptr<otterbrix::wrapper_dispatcher_t> wrapper_dispatcher_;
    };

} // namespace otterbrix
