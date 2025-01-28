#include "tape_ref.hpp"

#include <components/document/impl/document.hpp>

namespace components::document::internal {

    constexpr uint64_t JSON_VALUE_MASK = 0x00FFFFFFFFFFFFFF;

    tape_ref::tape_ref() noexcept
        : doc_{nullptr}
        , json_index_{0} {}
    tape_ref::tape_ref(const impl::base_document* doc, size_t json_index) noexcept
        : doc_{doc}
        , json_index_{json_index} {}
    types::physical_type tape_ref::tape_ref_type() const noexcept {
        return static_cast<types::physical_type>(doc_->get_tape(json_index_) >> 56);
    }
    uint64_t tape_ref::tape_value() const noexcept { return doc_->get_tape(json_index_) & JSON_VALUE_MASK; }

    bool tape_ref::is_float() const noexcept {
        constexpr auto tape_float = uint8_t(types::physical_type::FLOAT);
        return reinterpret_cast<const uint8_t*>(&doc_->get_tape(json_index_))[7] == tape_float;
    }

    bool tape_ref::is_double() const noexcept {
        constexpr uint64_t tape_double = uint64_t(types::physical_type::DOUBLE) << 56;
        return doc_->get_tape(json_index_) == tape_double;
    }

    bool tape_ref::is_int8() const noexcept {
        constexpr auto tape_int32 = uint8_t(types::physical_type::INT8);
        return reinterpret_cast<const uint8_t*>(&doc_->get_tape(json_index_))[7] == tape_int32;
    }

    bool tape_ref::is_int16() const noexcept {
        constexpr auto tape_int32 = uint8_t(types::physical_type::INT16);
        return reinterpret_cast<const uint8_t*>(&doc_->get_tape(json_index_))[7] == tape_int32;
    }

    bool tape_ref::is_int32() const noexcept {
        constexpr auto tape_int32 = uint8_t(types::physical_type::INT32);
        return reinterpret_cast<const uint8_t*>(&doc_->get_tape(json_index_))[7] == tape_int32;
    }

    bool tape_ref::is_int64() const noexcept {
        constexpr uint64_t tape_int64 = uint64_t(types::physical_type::INT64) << 56;
        return doc_->get_tape(json_index_) == tape_int64;
    }

    bool tape_ref::is_int128() const noexcept {
        constexpr uint64_t tape_int128 = uint64_t(types::physical_type::INT128) << 56;
        return doc_->get_tape(json_index_) == tape_int128;
    }

    bool tape_ref::is_uint8() const noexcept {
        constexpr auto tape_int32 = uint8_t(types::physical_type::UINT8);
        return reinterpret_cast<const uint8_t*>(&doc_->get_tape(json_index_))[7] == tape_int32;
    }

    bool tape_ref::is_uint16() const noexcept {
        constexpr auto tape_int32 = uint8_t(types::physical_type::UINT16);
        return reinterpret_cast<const uint8_t*>(&doc_->get_tape(json_index_))[7] == tape_int32;
    }

    bool tape_ref::is_uint32() const noexcept {
        constexpr auto tape_uint32 = uint8_t(types::physical_type::UINT32);
        return reinterpret_cast<const uint8_t*>(&doc_->get_tape(json_index_))[7] == tape_uint32;
    }

    bool tape_ref::is_uint64() const noexcept {
        constexpr uint64_t tape_uint64 = uint64_t(types::physical_type::UINT64) << 56;
        return doc_->get_tape(json_index_) == tape_uint64;
    }

    bool tape_ref::is_bool() const noexcept {
        constexpr uint64_t tape_bool = uint8_t(types::physical_type::BOOL);
        return reinterpret_cast<const uint8_t*>(&doc_->get_tape(json_index_))[7] == tape_bool;
    }

    bool tape_ref::is_null_on_tape() const noexcept {
        constexpr uint64_t tape_null = uint64_t(types::physical_type::NA) << 56;
        return doc_->get_tape(json_index_) == tape_null;
    }

    uint32_t tape_ref::get_string_length() const noexcept {
        size_t string_buf_index = size_t(tape_value());
        uint32_t len;
        std::memcpy(&len, &doc_->get_string_buf(string_buf_index), sizeof(len));
        return len;
    }

    const char* tape_ref::get_c_str() const noexcept {
        size_t string_buf_index = size_t(tape_value());
        return reinterpret_cast<const char*>(&doc_->get_string_buf(string_buf_index + sizeof(uint32_t)));
    }

    std::string_view tape_ref::get_string_view() const noexcept {
        return std::string_view(get_c_str(), get_string_length());
    }
} // namespace components::document::internal
