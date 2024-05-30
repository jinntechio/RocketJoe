#pragma once

#include <core/handler_by_id.hpp>

namespace services::memory_storage {

    enum class route : uint64_t
    {
        execute_ql,
        execute_ql_finish,
        execute_plan_finish,
        load,
        load_finish
    };

    constexpr uint64_t handler_id(route type) { return handler_id(group_id_t::memory_storage, type); }

} // namespace services::memory_storage
