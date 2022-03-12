#ifndef compiler_h
#define compiler_h
#include "bytecode.h"
#include "scanner.h"
#include "symbols.h"
#include <stdarg.h>
/**
 * Convert member function pointer to callable method
 */
#define PARSER_FN(method) (this->*method)

#define PRECEDENCE_GREATER_THAN(token) ((enum Precedence) ((int)this->get_binary_precedence (token) + 1))

class Compiler;

typedef void (Compiler::*Parser) ();

enum Precedence {
        PREC_NONE,
        PREC_ASSIGNMENT,
        PREC_LOGICAL,
        PREC_COMPARISON,
        PREC_PRODUCT,
        PREC_TERM,
        PREC_UNARY,
        PREC_PRIMARY
};

struct ParseRule {
        Parser binary_parser;
        Parser unary_parser;
        enum Precedence binary_prec;
        enum Precedence unary_prec;
};

class Compiler {
    public:
        Compiler (char *src_code);
        Compiler (FILE *source_fp);
        bool compile ();

    private:
        struct ParseRule rules[100];
        struct token curr_token;
        struct token prev_token;

        Scanner *scanner;
        Symbols *symbols;
        Bytecode *bytecode;

        bool has_error;

        bool match (enum token_t t);

        struct token peekToken();

        enum token_t peek ();
        enum token_t previous ();

        void advance ();
        void consume (enum token_t t, char *error_message, ...);

        void parse_statement ();

        void parse_condition ();
        void parse_while ();
        void parse_for ();
        void parse_block ();
        void parse_expression ();

        Parser get_binary_parser (enum token_t t);
        Parser get_unary_parser (enum token_t t);

        enum Precedence get_binary_precedence (enum token_t t);
        enum Precedence get_unary_precedence (enum token_t t);

        void parse_error (char *error, ...);
        void parse_precedence (enum Precedence prec);
        void parse_primary ();
        void parse_unary ();
        void parse_terms ();
        void parse_products ();
        void parse_comparison ();
        void parse_logical ();
        void parse_assignment ();
};

#endif