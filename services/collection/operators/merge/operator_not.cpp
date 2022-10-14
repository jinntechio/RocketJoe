#include "operator_not.hpp"

namespace services::collection::operators::merge {

    operator_not_t::operator_not_t(context_collection_t* context)
        : operator_merge_t(context) {
    }

    void operator_not_t::on_merge_impl(planner::transaction_context_t* transaction_context) {
        //todo: optimize merge
        if (left_ && left_->output()) {
            output_ = make_operator_data(context_->resource());
            const auto &left_documents = left_->output()->documents();
            for (const auto &document : context_->storage()) {
                auto it = std::find_if(left_documents.cbegin(), left_documents.cend(), [&document](const document_ptr &doc) {
                    return get_document_id(doc) == document.first;
                });
                if (it == left_documents.cend()) {
                    output_->append(document.second);
                }
            }
        }
    }

} // namespace services::collection::operators::merge