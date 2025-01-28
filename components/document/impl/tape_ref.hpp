#pragma once

#include <components/document/impl/document.hpp>
#include <components/types/types.hpp>
#include <cstring>

namespace components::document {
    namespace impl {
        class base_document;
    } // namespace impl

    namespace internal {

        class tape_ref {
        public:
            tape_ref() noexcept;
            tape_ref(const impl::base_document* doc, size_t json_index) noexcept;
            types::physical_type tape_ref_type() const noexcept;
            uint64_t tape_value() const noexcept;

            bool is_float() const noexcept;
            bool is_double() const noexcept;
            bool is_int8() const noexcept;
            bool is_int16() const noexcept;
            bool is_int32() const noexcept;
            bool is_int64() const noexcept;
            bool is_int128() const noexcept;
            bool is_uint8() const noexcept;
            bool is_uint16() const noexcept;
            bool is_uint32() const noexcept;
            bool is_uint64() const noexcept;
            bool is_bool() const noexcept;
            bool is_null_on_tape() const noexcept; // different name to avoid clash with is_null.

            template<typename T, typename std::enable_if<(sizeof(T) < sizeof(uint64_t)), uint8_t>::type = 0>
            T next_tape_value() const noexcept {
                T x;
                std::memcpy(&x, &doc_->get_tape(json_index_), sizeof(T));
                return x;
            }

            template<typename T, typename std::enable_if<(sizeof(T) >= sizeof(uint64_t)), uint8_t>::type = 1>
            T next_tape_value() const noexcept {
                static_assert(sizeof(T) == sizeof(uint64_t) || sizeof(T) == 2 * sizeof(uint64_t),
                              "next_tape_value() template parameter must be 8, 16, 32, 64 or 128-bit");
                T x;
                std::memcpy(&x, &doc_->get_tape(json_index_ + 1), sizeof(T));
                return x;
            }

            uint32_t get_string_length() const noexcept;
            const char* get_c_str() const noexcept;
            std::string_view get_string_view() const noexcept;
            bool usable() const noexcept {
                return doc_ && json_index_ < doc_->size();
            } // when the document pointer is null, this tape_ref is uninitialized (should not be accessed).

            const impl::base_document* doc_;

            size_t json_index_;
        };

    } // namespace internal
} // namespace components::document
