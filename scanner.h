#ifndef scanner_h
#define scanner_h

#include <stdlib.h>

enum token_t {

        PLUS,
        PLUS_EQUAL,
        MINUS,
        MINUS_EQUAL,
        MULT,
        MULT_EQUAL,
        DIV,
        DIV_EQUAL,
        PERCENT,
        MOD_EQUAL,
        LPAREN,
        RPAREN,
        LBRACKET,
        RBRACKET,
        LBRACE,
        RBRACE,
        AMPERSAND,
        PIPE,
        GT,
        LT,
        GTEQUAL,
        LTEQUAL,
        EQUAL,
        EQUAL_EQUAL,
        BANG,
        BANG_EQUAL,
        BIT_NOT,
        BIT_AND,
        BIT_OR,
        AND,
        OR,
        IF,
        ELSE,
        WHILE,
        FOR,
        RETURN,
        FUNC,
        SEMICOLON,
        INT,
        FLOAT,
        DOUBLE,
        STRING,
        IDENTIFIER,
        END
};
struct token {
        char *name;
        char *code_line;
        size_t len;
        union {
                int i;
                float f;
                double d;
        };
        enum token_t type;
        size_t line;
        size_t col;
};

class Scanner {
    public:
        Scanner (char *src_code);
        struct token scan_token ();
        int line_no;
        int col_no;
        char *curr_line;
        bool has_errors;

    private:
        char *source;
        char *curr;
        void scan_error (const char *message, ...);
        void highlight_line (size_t start_col, size_t end_col);
        bool match (char c);
        bool at_end ();
        char peek ();
        void advance ();
        void skip_comment ();
        bool is_alpha (char c);
        bool is_numeric (char c);
        bool is_valid_identifier_char (char c);
        enum token_t match_keyword (char *keyword, int length);
        struct token match_identifier ();
        struct token match_number ();
        struct token make_token (enum token_t);
};
#endif