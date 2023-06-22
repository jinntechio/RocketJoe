#pragma once

#include "token.hpp"

namespace components::sql {

    class lexer_t
    {
    public:
        lexer_t(const char* const query_begin, const char* const query_end);
        explicit lexer_t(std::string_view query);

        token_t next_token();

    private:
        const char* const begin_;
        const char* const end_;
        const char* pos_;

        token_t next_token_();
        inline bool check_pos_(char c);
    };

} // namespace components::sql
