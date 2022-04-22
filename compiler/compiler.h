#ifndef compiler_h
#define compiler_h
#include "bytecode.h"
#include "function.h"
#include "scanner.h"
#include "symbols.h"
#include <stdarg.h>
#include <vector>
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
        ~Compiler ();
        Function *compile ();
        Function *link ();

    private:
        std::unordered_map<int32_t, std::string> call_placeholders;
        std::unordered_map<std::string, Function *> symbol_to_function;

        int32_t next_placeholder_value;
        int32_t resolve_function_placeholder (char *func_name, size_t len);
        Function *resolve_placeholder (int32_t placeholder);
        void add_symbol (char *symbol, size_t len, size_t address);

        std::string convert_to_string (char *s, size_t len);
        struct ParseRule rules[100];
        struct token curr_token;
        struct token prev_token;

        Scanner *scanner;
        Symbols *symbols;
        Function *function;
        std::vector<Function *> functions;

        bool has_error;

        void setup (char *src_code);

        bool match (enum token_t t);
        bool at_end ();
        struct token peek_token ();

        enum token_t peek ();
        enum token_t previous ();

        void advance ();
        void consume (enum token_t t, const char *error_message, ...);

        void variable_check_before_assignment (char *variable_name, size_t len, struct token token, struct token assign_op);
        Function *find_function_by_name(char *func_name, size_t len);

        void resolve_call_statement(char *func_name, size_t len, int param_count);

        void parse_statement ();

        void parse_condition ();
        void parse_while ();
        void parse_for ();
        void parse_block ();
        void parse_expression ();
        void parse_expression_statement ();
        void parse_function_statement ();
        void push_block_scope ();
        void pop_block_scope ();

        Parser get_binary_parser (enum token_t t);
        Parser get_unary_parser (enum token_t t);

        enum Precedence get_binary_precedence (enum token_t t);
        enum Precedence get_unary_precedence (enum token_t t);

        void parse_error (const char *error, struct token t, ...);
        void highlight_line (struct token t);
        void parse_precedence (enum Precedence prec);
        void parse_primary ();
        void parse_unary ();
        void parse_terms ();
        void parse_products ();
        void parse_comparison ();
        void parse_logical ();
        void parse_return();
};

#endif