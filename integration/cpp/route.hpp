#pragma once

#include <services/collection/route.hpp>
#include <services/disk/route.hpp>
#include <services/memory_storage/route.hpp>
#include <services/wal/route.hpp>

namespace otterbrix {

    namespace memory_storage {
        using services::memory_storage::handler_id;
        using services::memory_storage::route;
    } // namespace memory_storage

    namespace collection {
        using services::collection::handler_id;
        using services::collection::route;
    } // namespace collection

    namespace disk {
        using services::disk::handler_id;
        using services::disk::route;
    } // namespace disk

    namespace wal {
        using services::wal::handler_id;
        using services::wal::route;
    } // namespace wal

} // namespace otterbrix
