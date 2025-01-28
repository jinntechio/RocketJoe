#pragma once

#include <cassert>
#include <chrono>
#include <memory>
#include <variant>

#include "types.hpp"

namespace components::types {

    class logical_value_t {
    public:
        explicit logical_value_t(complex_logical_type type = logical_type::NA);

        template<typename T>
        explicit logical_value_t(T value);
        logical_value_t(const logical_value_t& other);
        logical_value_t(logical_value_t&& other) noexcept;
        logical_value_t& operator=(const logical_value_t& other);
        logical_value_t& operator=(logical_value_t&& other) noexcept;
        ~logical_value_t() = default;

        const complex_logical_type& type() const noexcept;
        template<typename T>
        T value() const;
        bool is_null() const noexcept;
        logical_value_t cast_as(const complex_logical_type& type) const;
        void set_alias(const std::string& alias);

        bool operator==(const logical_value_t& rhs) const;
        bool operator!=(const logical_value_t& rhs) const;
        bool operator<(const logical_value_t& rhs) const;
        bool operator>(const logical_value_t& rhs) const;
        bool operator<=(const logical_value_t& rhs) const;
        bool operator>=(const logical_value_t& rhs) const;

        const std::vector<logical_value_t>& children() const;

        static logical_value_t create_struct(const std::vector<logical_value_t>& fields);
        static logical_value_t create_struct(const complex_logical_type& type,
                                             const std::vector<logical_value_t>& struct_values);
        static logical_value_t create_array(const complex_logical_type& internal_type,
                                            const std::vector<logical_value_t>& values);
        static logical_value_t create_numeric(const complex_logical_type& type, int64_t value);
        static logical_value_t create_decimal(int64_t value, uint8_t width, uint8_t scale);
        static logical_value_t create_map(const complex_logical_type& key_type,
                                          const complex_logical_type& value_type,
                                          const std::vector<logical_value_t>& keys,
                                          const std::vector<logical_value_t>& values);
        static logical_value_t create_map(const complex_logical_type& child_type,
                                          const std::vector<logical_value_t>& values);
        static logical_value_t create_list(const complex_logical_type& type,
                                           const std::vector<logical_value_t>& values);

    private:
        complex_logical_type type_;

        std::variant<nullptr_t,
                     bool,
                     int8_t,
                     int16_t,
                     int32_t,
                     int64_t,
                     uint8_t,
                     uint16_t,
                     uint32_t,
                     uint64_t,
                     float,
                     double,
                     std::chrono::nanoseconds,
                     std::chrono::microseconds,
                     std::chrono::milliseconds,
                     std::chrono::seconds,
                     void*,

                     // everything bigger then 8 bytes or has no fixed size is allocated on the heap

                     std::unique_ptr<std::string>,
                     std::unique_ptr<std::vector<logical_value_t>> // nested
                     >
            value_;
    };

    template<typename T>
    logical_value_t::logical_value_t(T value)
        : type_(complex_logical_type::to_logical_type<T>())
        , value_(value) {
        assert(type_ != logical_type::INVALID);
    }

    template<typename T>
    T logical_value_t::value() const {
        assert(false);
    }

    template<>
    inline logical_value_t::logical_value_t<std::string>(std::string value)
        : type_(logical_type::STRING_LITERAL)
        , value_(std::make_unique<std::string>(std::move(value))) {}

    // TODO: add checks for types
    template<>
    inline bool logical_value_t::value<bool>() const {
        return std::get<bool>(value_);
    }
    template<>
    inline uint8_t logical_value_t::value<uint8_t>() const {
        return std::get<uint8_t>(value_);
    }
    template<>
    inline int8_t logical_value_t::value<int8_t>() const {
        return std::get<int8_t>(value_);
    }
    template<>
    inline uint16_t logical_value_t::value<uint16_t>() const {
        return std::get<uint16_t>(value_);
    }
    template<>
    inline int16_t logical_value_t::value<int16_t>() const {
        return std::get<int16_t>(value_);
    }
    template<>
    inline uint32_t logical_value_t::value<uint32_t>() const {
        return std::get<uint32_t>(value_);
    }
    template<>
    inline int32_t logical_value_t::value<int32_t>() const {
        return std::get<int32_t>(value_);
    }
    template<>
    inline uint64_t logical_value_t::value<uint64_t>() const {
        return std::get<uint64_t>(value_);
    }
    template<>
    inline int64_t logical_value_t::value<int64_t>() const {
        return std::get<int64_t>(value_);
    }
    template<>
    inline float logical_value_t::value<float>() const {
        return std::get<float>(value_);
    }
    template<>
    inline double logical_value_t::value<double>() const {
        return std::get<double>(value_);
    }
    template<>
    inline std::chrono::nanoseconds logical_value_t::value<std::chrono::nanoseconds>() const {
        return std::get<std::chrono::nanoseconds>(value_);
    }
    template<>
    inline std::chrono::microseconds logical_value_t::value<std::chrono::microseconds>() const {
        return std::get<std::chrono::microseconds>(value_);
    }
    template<>
    inline std::chrono::milliseconds logical_value_t::value<std::chrono::milliseconds>() const {
        return std::get<std::chrono::milliseconds>(value_);
    }
    template<>
    inline std::chrono::seconds logical_value_t::value<std::chrono::seconds>() const {
        return std::get<std::chrono::seconds>(value_);
    }
    template<>
    inline void* logical_value_t::value<void*>() const {
        return std::get<void*>(value_);
    }
    template<>
    inline std::string* logical_value_t::value<std::string*>() const {
        return std::get<std::unique_ptr<std::string>>(value_).get();
    }
    template<>
    inline std::vector<logical_value_t>* logical_value_t::value<std::vector<logical_value_t>*>() const {
        return std::get<std::unique_ptr<std::vector<logical_value_t>>>(value_).get();
    }

} // namespace components::types