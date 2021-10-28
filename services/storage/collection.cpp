#include "collection.hpp"
#include "database.hpp"

#include "protocol/insert.hpp"
#include "protocol/request_select.hpp"
#include "result_insert_one.hpp"
#include "result.hpp"
#include "components/storage/mutable/mutable_dict.h"
#include "components/storage/mutable/mutable_array.h"

using ::storage::impl::value_type;
using ::storage::impl::mutable_dict_t;
using ::storage::impl::mutable_array_t;

namespace services::storage {

collection_t::collection_t(database_ptr database, log_t& log)
    : goblin_engineer::abstract_service(database, "collection")
    , log_(log.clone())
    , structure_(mutable_dict_t::new_dict())
    , index_(mutable_dict_t::new_dict())
{
    /// add_handler(collection::select, &collection_t::select);
    add_handler(collection::insert, &collection_t::insert);
    //add_handler(collection::erase, &collection_t::erase);
    add_handler(collection::search, &collection_t::search);
    add_handler(collection::find, &collection_t::find);
    add_handler(collection::size, &collection_t::size);
}

collection_t::~collection_t() {
    storage_.clear();
}

void collection_t::insert(session_t& session, std::string& collection, document_t& document) {
    log_.debug("collection_t::insert : {}", collection);
    insert_(std::move(document));
    auto dispatcher = addresses("dispatcher");
    log_.debug("dispatcher : {}", dispatcher->type());
    auto database = addresses("database");
    log_.debug("database : {}", database->type());
    goblin_engineer::send(dispatcher, self(), "insert_finish", session, result_insert_one(true));
}

auto collection_t::get(components::storage::conditional_expression& cond) -> void {
//    for (auto& i : *this) {
//        if (cond.check(i.second)) {
//            /// return py::cast(tmp);
//        }
//    }
}

auto collection_t::search(const session_t &session, const std::string &collection, query_ptr cond) -> void {
    log_.debug("collection {}::search", collection);
    auto dispatcher = addresses("dispatcher");
    log_.debug("dispatcher : {}", dispatcher->type());
    auto database = addresses("database");
    log_.debug("database : {}", database->type());
    goblin_engineer::send(dispatcher, self(), "search_finish", session, result_find(search_(std::move(cond))));
}

auto collection_t::find(const session_t& session, const std::string &collection, const document_t &cond) -> void {
    log_.debug("collection {}::find", collection);
    auto dispatcher = addresses("dispatcher");
    log_.debug("dispatcher : {}", dispatcher->type());
    auto database = addresses("database");
    log_.debug("database : {}", database->type());
    goblin_engineer::send(dispatcher, self(), "find_finish", session, result_find(search_(parse_condition(cond))));
}

auto collection_t::all() -> void {
    /*
        py::list tmp;
        wrapper_document_ptr doc;
        for (auto &i:*ptr_) {
            auto result = cache_.find(i.first);
            if (result == cache_.end()) {
                auto it = cache_.emplace(i.first, wrapper_document_ptr(new wrapper_document(i.second.get())));
                doc = it.first->second;
            } else {
                doc = result->second;
            }

            tmp.append(doc);
        }
        return tmp;
         */
}
/*
    void collection_t::insert_many(py::iterable iterable) {
        auto iter = py::iter(iterable);
        for(;iter!= py::iterator::sentinel();++iter) {
            auto document = *iter;
            auto is_document = py::isinstance<py::dict>(document);
            if (is_document) {
                auto doc = friedrichdb::core::make_document();
                to_document(document,*doc);
                insert_(document["_id"].cast<std::string>(), std::move(doc));
            }
        }
    }
*/
auto collection_t::size(session_t& session, std::string& collection) -> void {
    log_.debug("collection {}::size", collection);
    auto dispatcher = addresses("dispatcher");
    log_.debug("dispatcer : {}", dispatcher->type());
    auto database = addresses("database");
    log_.debug("database : {}", database->type());
    goblin_engineer::send(dispatcher, self(), "size_finish", session, result_size(size_()));
}

void collection_t::update(components::storage::document_t& fields, components::storage::conditional_expression& cond) {
    /*
        auto is_document = py::isinstance<py::dict>(fields);
        auto is_none = fields.is(py::none());
        if (is_none and is_document) {
            throw pybind11::type_error("fields is none or not dict  ");
        }

        auto is_not_none_cond = !cond.is(py::none());

        if (is_not_none_cond) {
            wrapper_document_ptr doc;
            for (auto &i:*ptr_) {
                auto result = cache_.find(i.first);
                if (result == cache_.end()) {
                    auto it = cache_.emplace(i.first, wrapper_document_ptr(new wrapper_document(i.second.get())));
                    doc = it.first->second;
                } else {
                    doc = result->second;
                }

                update_document(fields, *(doc->raw()));
            }
            return;
        }

        throw pybind11::type_error(" note cond ");
*/
}

void collection_t::remove(components::storage::conditional_expression& cond) {
    /*
        auto is_not_empty = !cond.is(py::none());
        if (is_not_empty) {
            wrapper_document_ptr tmp;
            std::string key;
            for (auto &i:*ptr_) {
                auto result = cache_.find(i.first);
                if (result == cache_.end()) {
                    auto it = cache_.emplace(i.first, wrapper_document_ptr(new wrapper_document(i.second.get())));
                    tmp = it.first->second;
                    key = it.first->first;
                } else {
                    tmp = result->second;
                    key= result->first;
                }
                if (cond(tmp).cast<bool>()) {
                    cache_.erase(key);
                    ptr_->remove(key);
                }
            }
        }
         */
}

void collection_t::drop() {
    drop_();
}

std::string collection_t::gen_id() const {
    return std::string("0"); //todo
}

void collection_t::insert_(document_t&& document, int version) {
    auto id = document.is_exists("_id") ? document.get_string("_id") : gen_id();
    auto index = mutable_dict_t::new_dict();
    for (auto it = document.begin(); it; ++it) {
        auto key = it.key()->as_string();
        auto value = it.value();
        if (!structure_->get(key)) {
            structure_->set(key, document_t::get_msgpack_type(value));
        }
        auto offset = storage_.str().size();
        pack_(value);
        auto field = mutable_array_t::new_array();
        field->append(offset);
        field->append(storage_.str().size() - offset);
        if (version) field->append(version);
        index->set(key, field);
    }
    index_->set(std::move(id), index);
}

void collection_t::pack_(const ::storage::impl::value_t *value) {
    if (value->type() == value_type::boolean) msgpack::pack(storage_, value->as_bool());
    if (value->is_unsigned()) msgpack::pack(storage_, value->as_unsigned());
    if (value->is_int()) msgpack::pack(storage_, value->as_int());
    if (value->is_double()) msgpack::pack(storage_, value->as_double());
    if (value->type() == value_type::string) msgpack::pack(storage_, static_cast<std::string>(value->as_string()).data());
//    if (value->type() == value_type::array) msgpack::pack(storage_, msgpack::object(document_t::get_msgpack_array(value->as_array())));
    if (value->type() == value_type::dict) {
        //todo
    }
}

document_t* collection_t::get_(const std::string& uid) {
    /*
        auto it = storage_.find(uid);
        if (it == storage_.end()) {
            return nullptr;
        } else {
            it->second.get();
        }
         */
}

std::size_t collection_t::size_() const {
    //        return storage_.size();
}

void collection_t::drop_() {
    //        storage_.clear();
}

std::vector<document_t *> collection_t::search_(query_ptr cond) {
    std::vector<document_t *> res;
    //        for (auto it = storage_.begin(); it != storage_.end(); ++it) {
    //            if (!cond || cond->check(it->second)) {
    //                res.push_back(&it->second);
    //            }
    //        }
    return res;
}

auto collection_t::remove_(const std::string& key) {
    //        storage_.erase(key);
}

#ifdef DEV_MODE
void collection_t::insert_test(document_t &&doc) {
    insert_(std::move(doc));
}

std::vector<document_t *> collection_t::search_test(query_ptr cond) {
    return search_(std::move(cond));
}

std::vector<document_t *> collection_t::find_test(const document_t &cond) {
    return search_(parse_condition(std::move(cond)));
}

std::string collection_t::get_structure_test() const {
    return structure_->to_json_string();
}

std::string collection_t::get_index_test() const {
    return index_->to_json_string();
}

std::string collection_t::get_data_test() const {
    return storage_.str();
}
#endif

} // namespace services::storage
