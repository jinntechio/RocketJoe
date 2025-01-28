#include "types.hpp"
#include "logical_value.hpp"

#include <cassert>

namespace components::types {

    complex_logical_type::complex_logical_type(logical_type type)
        : type_(type) {}

    complex_logical_type::complex_logical_type(logical_type type, std::unique_ptr<logical_type_extention> extention)
        : type_(type)
        , extention_(std::move(extention)) {}

    complex_logical_type::complex_logical_type(const complex_logical_type& other)
        : type_(other.type_) {
        if (other.extention_) {
            switch (other.extention_->type()) {
                case logical_type_extention::extention_type::GENERIC:
                    extention_ = std::make_unique<logical_type_extention>(*other.extention_.get());
                    break;
                case logical_type_extention::extention_type::ARRAY:
                    extention_ = std::make_unique<array_logical_type_extention>(
                        *static_cast<array_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::MAP:
                    extention_ = std::make_unique<map_logical_type_extention>(
                        *static_cast<map_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::LIST:
                    extention_ = std::make_unique<list_logical_type_extention>(
                        *static_cast<list_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::STRUCT:
                    extention_ = std::make_unique<struct_logical_type_extention>(
                        *static_cast<struct_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::DECIMAL:
                    extention_ = std::make_unique<decimal_logical_type_extention>(
                        *static_cast<decimal_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::ENUM:
                    extention_ = std::make_unique<enum_logical_type_extention>(
                        *static_cast<enum_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::USER:
                    extention_ = std::make_unique<user_logical_type_extention>(
                        *static_cast<user_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::FUNCTION:
                    extention_ = std::make_unique<function_logical_type_extention>(
                        *static_cast<function_logical_type_extention*>(other.extention_.get()));
                    break;
                default:
                    assert(false && "complex_logical_type copy: unimplemented extention type");
            }
        }
    }

    complex_logical_type& complex_logical_type::operator=(const complex_logical_type& other) {
        type_ = other.type_;
        if (other.extention_) {
            switch (other.extention_->type()) {
                case logical_type_extention::extention_type::GENERIC:
                    extention_ = std::make_unique<logical_type_extention>(*other.extention_.get());
                    break;
                case logical_type_extention::extention_type::ARRAY:
                    extention_ = std::make_unique<array_logical_type_extention>(
                        *static_cast<array_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::MAP:
                    extention_ = std::make_unique<map_logical_type_extention>(
                        *static_cast<map_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::LIST:
                    extention_ = std::make_unique<list_logical_type_extention>(
                        *static_cast<list_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::STRUCT:
                    extention_ = std::make_unique<struct_logical_type_extention>(
                        *static_cast<struct_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::DECIMAL:
                    extention_ = std::make_unique<decimal_logical_type_extention>(
                        *static_cast<decimal_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::ENUM:
                    extention_ = std::make_unique<enum_logical_type_extention>(
                        *static_cast<enum_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::USER:
                    extention_ = std::make_unique<user_logical_type_extention>(
                        *static_cast<user_logical_type_extention*>(other.extention_.get()));
                    break;
                case logical_type_extention::extention_type::FUNCTION:
                    extention_ = std::make_unique<function_logical_type_extention>(
                        *static_cast<function_logical_type_extention*>(other.extention_.get()));
                    break;
                default:
                    assert(false && "complex_logical_type copy: unimplemented extention type");
            }
        }
        return *this;
    }

    bool complex_logical_type::operator==(const complex_logical_type& rhs) const {
        return type_ == rhs.type_;
        // TODO: also compare extentions
        //return type_ == rhs.type_ && *extention_.get() == *rhs.extention_.get();
    }

    bool complex_logical_type::operator!=(const complex_logical_type& rhs) const { return !(*this == rhs); }

    size_t complex_logical_type::size() const noexcept {
        switch (type_) {
            case logical_type::NA:
                return 1;
            case logical_type::BIT:
            case logical_type::VALIDITY:
            case logical_type::BOOLEAN:
                return sizeof(bool);
            case logical_type::TINYINT:
                return sizeof(int8_t);
            case logical_type::SMALLINT:
                return sizeof(int16_t);
            case logical_type::INTEGER:
                return sizeof(int32_t);
            case logical_type::BIGINT:
            case logical_type::TIMESTAMP_SEC:
            case logical_type::TIMESTAMP_MS:
            case logical_type::TIMESTAMP_US:
            case logical_type::TIMESTAMP_NS:
                return sizeof(int64_t);
            case logical_type::FLOAT:
                return sizeof(float);
            case logical_type::DOUBLE:
                return sizeof(double);
            case logical_type::UTINYINT:
                return sizeof(uint8_t);
            case logical_type::USMALLINT:
                return sizeof(uint16_t);
            case logical_type::UINTEGER:
                return sizeof(uint32_t);
            case logical_type::UBIGINT:
                return sizeof(uint64_t);
            case logical_type::STRING_LITERAL:
                return sizeof(std::string_view);
            case logical_type::POINTER:
                return sizeof(void*);
            case logical_type::LIST:
                return sizeof(list_entry_t);
            case logical_type::ARRAY:
            case logical_type::STRUCT:
                return 0; // no own payload
            default:
                assert(false && "complex_logical_type::objest_size: reached unsupported type");
                break;
        }
    }

    size_t complex_logical_type::align() const noexcept {
        switch (type_) {
            case logical_type::NA:
                return 1;
            case logical_type::BOOLEAN:
                return alignof(bool);
            case logical_type::TINYINT:
                return alignof(int8_t);
            case logical_type::SMALLINT:
                return alignof(int16_t);
            case logical_type::INTEGER:
                return alignof(int32_t);
            case logical_type::BIGINT:
            case logical_type::TIMESTAMP_SEC:
            case logical_type::TIMESTAMP_MS:
            case logical_type::TIMESTAMP_US:
            case logical_type::TIMESTAMP_NS:
                return alignof(int64_t);
            case logical_type::FLOAT:
                return alignof(float);
            case logical_type::DOUBLE:
                return alignof(double);
            case logical_type::UTINYINT:
                return alignof(uint8_t);
            case logical_type::USMALLINT:
                return alignof(uint16_t);
            case logical_type::UINTEGER:
                return alignof(uint32_t);
            case logical_type::UBIGINT:
            case logical_type::VALIDITY:
                return alignof(uint64_t);
            case logical_type::STRING_LITERAL:
                return alignof(std::string_view);
            case logical_type::POINTER:
                return alignof(void*);
            case logical_type::LIST:
                return alignof(list_entry_t);
            case logical_type::ARRAY:
            case logical_type::STRUCT:
                return 0; // no own payload
            default:
                assert(false && "complex_logical_type::objest_size: reached unsupported type");
                break;
        }
    }

    physical_type complex_logical_type::to_physical_type() const {
        switch (type_) {
            case logical_type::NA:
            case logical_type::BOOLEAN:
                return physical_type::BOOL;
            case logical_type::TINYINT:
                return physical_type::INT8;
            case logical_type::UTINYINT:
                return physical_type::UINT8;
            case logical_type::SMALLINT:
                return physical_type::INT16;
            case logical_type::USMALLINT:
                return physical_type::UINT16;
            case logical_type::INTEGER:
                return physical_type::INT32;
            case logical_type::UINTEGER:
                return physical_type::UINT32;
            case logical_type::BIGINT:
            case logical_type::TIMESTAMP_SEC:
            case logical_type::TIMESTAMP_MS:
            case logical_type::TIMESTAMP_US:
            case logical_type::TIMESTAMP_NS:
                return physical_type::INT64;
            case logical_type::UBIGINT:
                return physical_type::UINT64;
            case logical_type::UHUGEINT:
                return physical_type::UINT128;
            case logical_type::HUGEINT:
            case logical_type::UUID:
                return physical_type::INT128;
            case logical_type::FLOAT:
                return physical_type::FLOAT;
            case logical_type::DOUBLE:
                return physical_type::DOUBLE;
            case logical_type::STRING_LITERAL:
                return physical_type::STRING;
            case logical_type::DECIMAL:
                return physical_type::INT64;
            case logical_type::VALIDITY:
                return physical_type::BIT;
            case logical_type::ARRAY:
                return physical_type::ARRAY;
            case logical_type::STRUCT:
                return physical_type::STRUCT;
            case logical_type::LIST:
                return physical_type::LIST;
            default:
                return physical_type::INVALID;
        }
    }

    void complex_logical_type::set_alias(const std::string& alias) {
        if (extention_) {
            extention_->set_alias(alias);
        } else {
            extention_ =
                std::make_unique<logical_type_extention>(logical_type_extention::extention_type::GENERIC, alias);
        }
    }

    bool complex_logical_type::has_alias() const {
        if (extention_ && !extention_->alias().empty()) {
            return true;
        }
        return false;
    }

    const std::string& complex_logical_type::alias() const {
        assert(extention_);
        return extention_->alias();
    }

    const std::string& complex_logical_type::child_name(uint64_t index) const {
        assert(type_ == logical_type::STRUCT);
        return static_cast<struct_logical_type_extention*>(extention_.get())->child_types()[index].alias();
    }

    bool complex_logical_type::is_unnamed() const { return extention_->alias().empty(); }

    bool complex_logical_type::is_nested() const {
        switch (type_) {
            case logical_type::STRUCT:
            case logical_type::LIST:
            case logical_type::ARRAY:
                return true;
            default:
                return false;
        }
    }

    const complex_logical_type& complex_logical_type::child_type() const {
        assert(type_ == logical_type::ARRAY || type_ == logical_type::LIST);
        if (type_ == logical_type::ARRAY) {
            return static_cast<array_logical_type_extention*>(extention_.get())->internal_type();
        }
        if (type_ == logical_type::LIST) {
            return static_cast<list_logical_type_extention*>(extention_.get())->node();
        }

        return logical_type::INVALID;
    }

    logical_type_extention* complex_logical_type::extention() const noexcept { return extention_.get(); }

    const std::vector<complex_logical_type>& complex_logical_type::child_types() const {
        assert(extention_);
        return static_cast<struct_logical_type_extention*>(extention_.get())->child_types();
    }

    bool complex_logical_type::type_is_constant_size(logical_type type) {
        return type >= logical_type::BOOLEAN && type <= logical_type::DOUBLE;
    }

    complex_logical_type complex_logical_type::create_decimal(uint8_t width, uint8_t scale) {
        assert(width >= scale);
        return complex_logical_type(logical_type::DECIMAL,
                                    std::make_unique<decimal_logical_type_extention>(width, scale));
    }

    complex_logical_type complex_logical_type::create_list(const complex_logical_type& internal_type) {
        return complex_logical_type(logical_type::LIST, std::make_unique<list_logical_type_extention>(internal_type));
    }

    complex_logical_type complex_logical_type::create_array(const complex_logical_type& internal_type,
                                                            size_t array_size) {
        return complex_logical_type(logical_type::ARRAY,
                                    std::make_unique<array_logical_type_extention>(internal_type, array_size));
    }

    complex_logical_type complex_logical_type::create_map(const complex_logical_type& key_type,
                                                          const complex_logical_type& value_type) {
        return complex_logical_type(logical_type::MAP,
                                    std::make_unique<map_logical_type_extention>(key_type, value_type));
    }

    complex_logical_type complex_logical_type::create_struct(const std::vector<complex_logical_type>& fields) {
        return complex_logical_type(logical_type::STRUCT, std::make_unique<struct_logical_type_extention>(fields));
    }

    logical_type_extention::logical_type_extention(extention_type t, std::string alias)
        : type_(t)
        , alias_(std::move(alias)) {}

    void logical_type_extention::set_alias(const std::string& alias) { alias_ = alias; }

    array_logical_type_extention::array_logical_type_extention(const complex_logical_type& type, uint64_t size)
        : logical_type_extention(extention_type::ARRAY)
        , items_type_(type)
        , size_(size) {}

    map_logical_type_extention::map_logical_type_extention(const complex_logical_type& key,
                                                           const complex_logical_type& value)
        : logical_type_extention(extention_type::MAP)
        , key_(key)
        , value_(value) {}

    list_logical_type_extention::list_logical_type_extention(complex_logical_type type)
        : logical_type_extention(extention_type::LIST)
        , type_(std::move(type)) {}

    struct_logical_type_extention::struct_logical_type_extention(const std::vector<complex_logical_type>& fields)
        : logical_type_extention(extention_type::STRUCT)
        , fields_(fields) {}

    const std::vector<complex_logical_type>& struct_logical_type_extention::child_types() const { return fields_; }

    decimal_logical_type_extention::decimal_logical_type_extention(uint8_t width, uint8_t scale)
        : logical_type_extention(extention_type::DECIMAL)
        , width_(width)
        , scale_(scale) {}

    enum_logical_type_extention::enum_logical_type_extention(std::vector<logical_value_t> entries)
        : logical_type_extention(extention_type::ENUM)
        , entries_(std::move(entries)) {}

    user_logical_type_extention::user_logical_type_extention(std::string catalog,
                                                             std::vector<logical_value_t> user_type_modifiers)
        : logical_type_extention(extention_type::USER)
        , catalog_(std::move(catalog))
        , user_type_modifiers_(std::move(user_type_modifiers)) {}

    function_logical_type_extention::function_logical_type_extention(complex_logical_type return_type,
                                                                     std::vector<complex_logical_type> arguments)
        : logical_type_extention(extention_type::FUNCTION)
        , return_type_(std::move(return_type))
        , argument_types_(std::move(arguments)) {}

} // namespace components::types