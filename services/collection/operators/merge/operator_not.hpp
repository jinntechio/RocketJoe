#pragma once

#include "operator_merge.hpp"

namespace services::collection::operators::merge {

    class operator_not_t : public operator_merge_t {
    public:
        explicit operator_not_t(context_collection_t* context);

    private:
        void on_merge_impl(planner::transaction_context_t* transaction_context) final;
    };

} // namespace services::collection::operators::merge
