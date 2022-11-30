#pragma once

#include "expression.hpp"
#include "key.hpp"

namespace components::expressions {

    class scalar_expression_t;
    using scalar_expression_ptr = boost::intrusive_ptr<scalar_expression_t>;

    class scalar_expression_t : public expression_i {
    public:
        using param_storage = std::variant<
            core::parameter_id_t,
            key_t,
            expression_ptr>;

        scalar_expression_t(const scalar_expression_t&) = delete;
        scalar_expression_t(scalar_expression_t&&) = default;

        scalar_expression_t(scalar_type type, const key_t& key);

        scalar_type type() const;
        const key_t& key() const;
        const std::vector<param_storage>& params() const;

        void append_param(const param_storage& param);

    private:
        scalar_type type_;
        key_t key_;
        std::vector<param_storage> params_;

        hash_t hash_impl() const final;
        std::string to_string_impl() const final;
        bool equal_impl(const expression_i* rhs) const final;
    };

    scalar_expression_ptr make_scalar_expression(scalar_type type, const key_t& key);
    scalar_expression_ptr make_scalar_expression(scalar_type type);
    scalar_expression_ptr make_scalar_expression(scalar_type type, const key_t& name, const key_t& key);

    scalar_type get_scalar_type(const std::string& key);
    bool is_scalar_type(const std::string& key);

} // namespace components::expressions