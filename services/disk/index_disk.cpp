#include "index_disk.hpp"

#include <msgpack/msgpack_encoder.hpp>

#include "core/b_plus_tree/msgpack_reader/msgpack_reader.hpp"

namespace services::disk {

    using namespace core::b_plus_tree;

    auto item_key_getter = [](const btree_t::item_data& item) -> btree_t::index_t {
        msgpack::unpacked msg;
        msgpack::unpack(msg, (char*) item.data, item.size, [](msgpack::type::object_type, std::size_t, void*) {
            return true;
        });
        return get_field(msg.get(), "/0");
    };
    auto id_getter = [](const btree_t::item_data& item) -> btree_t::index_t {
        msgpack::unpacked msg;
        msgpack::unpack(msg, (char*) item.data, item.size, [](msgpack::type::object_type, std::size_t, void*) {
            return true;
        });
        return get_field(msg.get(), "/1");
    };

    components::types::physical_value convert(const components::document::value_t& value) {
        switch (value.physical_type()) {
            case components::types::physical_type::BOOL:
                return components::types::physical_value(value.as_bool());
            case components::types::physical_type::UINT8:
                return components::types::physical_value(value.as<uint8_t>());
            case components::types::physical_type::INT8:
                return components::types::physical_value(value.as<int8_t>());
            case components::types::physical_type::UINT16:
                return components::types::physical_value(value.as<uint16_t>());
            case components::types::physical_type::INT16:
                return components::types::physical_value(value.as<int16_t>());
            case components::types::physical_type::UINT32:
                return components::types::physical_value(value.as<uint32_t>());
            case components::types::physical_type::INT32:
                return components::types::physical_value(value.as<int32_t>());
            case components::types::physical_type::UINT64:
                return components::types::physical_value(value.as<uint64_t>());
            case components::types::physical_type::INT64:
                return components::types::physical_value(value.as<int64_t>());
            case components::types::physical_type::UINT128:
            case components::types::physical_type::INT128:
                return components::types::physical_value(value.as_int128());
            case components::types::physical_type::FLOAT:
                return components::types::physical_value(value.as_float());
            case components::types::physical_type::DOUBLE:
                return components::types::physical_value(value.as_double());
            case components::types::physical_type::STRING:
                return components::types::physical_value(value.as_string());
            case components::types::physical_type::NA:
                return components::types::physical_value();
            default:
                assert(false && "unsupported type");
                return components::types::physical_value();
        }
    }

    index_disk_t::index_disk_t(const path_t& path, std::pmr::memory_resource* resource)
        : path_(path)
        , resource_(resource)
        , fs_(core::filesystem::local_file_system_t())
        , db_(std::make_unique<btree_t>(resource, fs_, path, item_key_getter)) {
        db_->load();
    }

    index_disk_t::~index_disk_t() = default;

    void index_disk_t::insert(const value_t& key, const document_id_t& value) {
        auto values = find(key);
        if (std::find(values.begin(), values.end(), value) == values.end()) {
            values.push_back(value);
            msgpack::sbuffer sbuf;
            msgpack::packer packer(sbuf);
            packer.pack_array(2);
            to_msgpack_(packer, key.get_element());
            packer.pack(value.to_string());
            db_->append(data_ptr_t(sbuf.data()), sbuf.size());
            db_->flush();
        }
    }

    void index_disk_t::remove(value_t key) {
        db_->remove_index(convert(key));
        db_->flush();
    }

    void index_disk_t::remove(const value_t& key, const document_id_t& doc) {
        auto values = find(key);
        if (!values.empty()) {
            values.erase(std::remove(values.begin(), values.end(), doc), values.end());
            msgpack::sbuffer sbuf;
            msgpack::packer packer(sbuf);
            packer.pack_array(2);
            to_msgpack_(packer, key.get_element());
            packer.pack(doc.to_string());
            db_->remove(data_ptr_t(sbuf.data()), sbuf.size());
            db_->flush();
        }
    }

    void index_disk_t::find(const value_t& value, result& res) const {
        auto index = convert(value);
        size_t count = db_->item_count(index);
        res.reserve(count);
        for (size_t i = 0; i < count; i++) {
            res.emplace_back(id_getter(db_->get_item(index, i)).value<components::types::physical_type::STRING>());
        }
    }

    index_disk_t::result index_disk_t::find(const value_t& value) const {
        index_disk_t::result res;
        find(value, res);
        return res;
    }

    void index_disk_t::lower_bound(const value_t& value, result& res) const {
        auto max_index = convert(value);
        db_->scan_ascending(
            std::numeric_limits<btree_t::index_t>::min(),
            max_index,
            size_t(-1),
            &res,
            [](void* data, size_t size) {
                return document_id_t(id_getter(btree_t::item_data{static_cast<data_ptr_t>(data), size})
                                         .value<components::types::physical_type::STRING>());
            },
            [&max_index](const auto& index, const auto&) { return index != max_index; });
    }

    index_disk_t::result index_disk_t::lower_bound(const value_t& value) const {
        index_disk_t::result res;
        lower_bound(value, res);
        return res;
    }

    void index_disk_t::upper_bound(const value_t& value, result& res) const {
        auto min_index = convert(value);
        db_->scan_decending(
            convert(value),
            std::numeric_limits<btree_t::index_t>::max(),
            size_t(-1),
            &res,
            [](void* data, size_t size) {
                return document_id_t(id_getter(btree_t::item_data{static_cast<data_ptr_t>(data), size})
                                         .value<components::types::physical_type::STRING>());
            },
            [&min_index](const auto& index, const auto&) { return index != min_index; });
    }

    index_disk_t::result index_disk_t::upper_bound(const value_t& value) const {
        index_disk_t::result res;
        upper_bound(value, res);
        return res;
    }

    void index_disk_t::drop() {
        db_.reset();
        core::filesystem::remove_directory(fs_, path_);
    }

} // namespace services::disk
