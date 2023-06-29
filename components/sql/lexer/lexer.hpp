#pragma once

#include <string>
#include "token.hpp"

namespace components::sql {

    class lexer_t
    {
    public:
        lexer_t(const char* const query_begin, const char* const query_end);
        explicit lexer_t(std::string_view query);
        explicit lexer_t(const std::string& query);

        token_t next_token();

    private:
        const char* const begin_;
        const char* const end_;
        const char* pos_;
        token_type prev_token_type_;
        token_type prev_significant_token_type_;

        token_t next_token_();
        inline bool check_pos_(char c);
        inline token_t create_quote_token_(const char* const token_begin, char quote, token_type type, token_type type_error);
        inline token_t create_comment_one_line_(const char* const token_begin);
        inline token_t create_comment_multi_line_(const char* const token_begin);
        inline token_t create_hex_or_bin_str(const char* const token_begin);
    };

} // namespace components::sql