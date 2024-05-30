#include "create_index_utils.hpp"

#include <components/index/disk/route.hpp>
#include <components/index/single_field_index.hpp>

namespace services::collection {

    bool try_update_index_compare(const components::document::document_view_t& doc,
                                  components::ql::create_index_ptr& index_ql) {
        switch (index_ql->index_compare_) {
            case core::type::undef: {
                // TODO use types based on statistic
                const auto deduced_type = doc.type_by_key(index_ql->keys_[0].as_string());
                if (deduced_type == core::type::undef) {
                    return false;
                }
                index_ql->index_compare_ = deduced_type;
                return true;
            }
            default: {
                // create_index already contains compare type
                // TODO add comparability validation with documents
                return true;
            }
        }
    }

    void create_index_impl(context_collection_t* context,
                           components::pipeline::context_t* pipeline_context,
                           components::ql::create_index_ptr index_ql,
                           bool is_pending) {
        trace(context->log(),
              "create_index_impl session: {}, index: {}",
              pipeline_context->session.data(),
              index_ql->name());
        switch (index_ql->index_type_) {
            case components::ql::index_type::single: {
                const bool index_exist = context->index_engine()->has_index(index_ql->name());
                const auto id_index = index_exist
                                          ? components::index::INDEX_ID_UNDEFINED
                                          : components::index::make_index<components::index::single_field_index_t>(
                                                context->index_engine(),
                                                index_ql->name(),
                                                index_ql->keys_);

                sessions::make_session(
                    context->sessions(),
                    pipeline_context->session,
                    index_ql->name(),
                    sessions::create_index_t{pipeline_context->current_message_sender, id_index, is_pending});
                pipeline_context->send(context->disk(),
                                       services::index::handler_id(services::index::route::create),
                                       std::move(*(index_ql.release())),
                                       context);
                trace(context->log(), "create_index_impl; pipeline context sent");
                break;
            }
            case components::ql::index_type::composite:
            case components::ql::index_type::multikey:
            case components::ql::index_type::hashed:
            case components::ql::index_type::wildcard: {
                trace(context->log(), "index_type not implemented");
                assert(false && "index_type not implemented");
                break;
            }
            case components::ql::index_type::no_valid: {
                trace(context->log(), "index_type not implemented");
                assert(false && "index_type not valid");
                break;
            }
        }
        trace(context->log(), "create_index_impl finished");
    }

    void create_pending_index_impl(context_collection_t* context,
                                   components::pipeline::context_t* pipeline_context,
                                   components::ql::create_index_ptr index_ql) {
        create_index_impl(context, pipeline_context, std::move(index_ql), true);
    }

    void process_pending_indexes(context_collection_t* context) {
        // If doc exist and pending indexes not empty -> add curr ql to indexes and run loop
        trace(context->log(), "Process pending indexes; size: {}", context->pending_indexes().size());

        assert(!(context->storage().empty()) && "Require non empty document storage");

        const auto doc_view = document_view_t(context->storage().begin()->second);
        for (auto& [index, pipeline_context] : context->pending_indexes()) {
            assert(index != nullptr && "pending index is null");
            if (!try_update_index_compare(doc_view, index)) {
                error(context->log(),
                      "Can't deduce compare type for index: {} with key {}",
                      index->name_,
                      index->keys_.front().as_string());
                continue;
            }
            create_pending_index_impl(context, pipeline_context.get(), std::move(index));
            trace(context->log(), "Process pending indexes; index proccessed");
        }
        context->pending_indexes().clear();
    }

} // namespace services::collection