#include "compiler.h"
#include "scanner.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
Compiler::Compiler (char *src_code)
{
        this->setup (src_code);
}

Compiler::Compiler (FILE *source_fp)
{
        fseek (source_fp, 0, SEEK_END);

        size_t file_size = ftell (source_fp);

        fseek (source_fp, 0, SEEK_SET);

        if (file_size == 0) {
                fprintf (stderr, "error: empty file\n");
                exit (EXIT_FAILURE);
        }

        char *source_buf = (char *)calloc ((file_size + 1), sizeof (char));

        fread (source_buf, file_size, 1, source_fp);

        this->setup (source_buf);
}

Compiler::~Compiler ()
{
}

void Compiler::setup (char *src_code)
{
#define RULE(token, binary, bprec, unary, uprec)                                                                       \
        this->rules[token] = {                                                                                         \
                .binary_parser = (binary),                                                                             \
                .unary_parser = (unary),                                                                               \
                .binary_prec = (bprec),                                                                                \
                .unary_prec = (uprec)                                                                                  \
        }
#define NONE_RULE(token) RULE (token, NULL, PREC_NONE, NULL, PREC_NONE)
        /*
         * indicates whether an compilation error has occurred.
         */
        this->has_error = false;
        this->scanner = new Scanner (src_code);

        /*
         * setup a bytecode output buffer object, this stores our emitted bytecode.
         */
        this->function = new Function ("script", 6);
        this->next_placeholder_value = 0;
        /*
         * initialize a symbols object to keep track of variable names/function
         * parameters
         */
        this->symbols = new Symbols (NULL, 0, 1);

        /*
         * scan a single token to setup the compiler for parsing
         */
        this->prev_token = (struct token){ 0 };
        this->curr_token = this->scanner->scan_token ();

        /**
         * Binary/Unary operators
         */
        RULE (PLUS, &Compiler::parse_terms, PREC_TERM, NULL, PREC_NONE);
        RULE (MINUS, &Compiler::parse_terms, PREC_TERM, &Compiler::parse_unary, PREC_UNARY);

        RULE (MULT, &Compiler::parse_products, PREC_PRODUCT, NULL, PREC_NONE);
        RULE (DIV, &Compiler::parse_products, PREC_PRODUCT, NULL, PREC_PRODUCT);

        RULE (AND, &Compiler::parse_logical, PREC_LOGICAL, NULL, PREC_NONE);
        RULE (OR, &Compiler::parse_logical, PREC_LOGICAL, NULL, PREC_NONE);

        RULE (LT, &Compiler::parse_comparison, PREC_COMPARISON, NULL, PREC_NONE);
        RULE (LTEQUAL, &Compiler::parse_comparison, PREC_COMPARISON, NULL, PREC_NONE);

        RULE (GT, &Compiler::parse_comparison, PREC_COMPARISON, NULL, PREC_NONE);
        RULE (GTEQUAL, &Compiler::parse_comparison, PREC_COMPARISON, NULL, PREC_NONE);

        RULE (EQUAL, NULL, PREC_ASSIGNMENT, NULL, PREC_ASSIGNMENT);
        RULE (PLUS_EQUAL, NULL, PREC_ASSIGNMENT, NULL, PREC_ASSIGNMENT);
        RULE (MINUS_EQUAL, NULL, PREC_ASSIGNMENT, NULL, PREC_ASSIGNMENT);
        RULE (MULT_EQUAL, NULL, PREC_ASSIGNMENT, NULL, PREC_ASSIGNMENT);
        RULE (DIV_EQUAL, NULL, PREC_ASSIGNMENT, NULL, PREC_ASSIGNMENT);

        /**
         * Primary tokens
         */
        RULE (LPAREN, NULL, PREC_NONE, &Compiler::parse_primary, PREC_PRIMARY);
        RULE (INT, NULL, PREC_PRIMARY, &Compiler::parse_primary, PREC_PRIMARY);
        RULE (FLOAT, NULL, PREC_NONE, &Compiler::parse_unary, PREC_PRIMARY);
        RULE (IDENTIFIER, NULL, PREC_NONE, &Compiler::parse_primary, PREC_PRIMARY);

        NONE_RULE (END);
        NONE_RULE (IF);
        NONE_RULE (FOR);
        NONE_RULE (ELSE);
        NONE_RULE (WHILE);
        NONE_RULE (FUNC);
        NONE_RULE (LBRACE);
        NONE_RULE (RBRACE);
        NONE_RULE (RPAREN);
        NONE_RULE (SEMICOLON);

#undef NONE_RULE
#undef RULE
}

void Compiler::highlight_line (struct token t)
{
        // print the line number pipe symbol seperator
        fprintf (stderr, "\t|\n");
        fprintf (stderr, " %lu\t| ", t.line);

        char *curr = t.code_line;

        // print the offending line of code
        for (int i = 0; *curr != '\n' && *curr != '\0'; i++, curr++) {
                fputc (*curr, stderr);
        }

        fprintf (stderr, "\n\t|");
        curr = t.code_line;

        // beneath the code, print the arrow/underline
        for (size_t i = 0; *curr != '\n' && *curr != '\0'; i++, curr++) {
                if (i < t.col)
                        fputc (' ', stderr);
                else if (t.col <= i && i < t.col + t.len)
                        fputc ('^', stderr);
                else
                        fputc ('~', stderr);
        }
        fputc ('\n', stderr);
}

Parser Compiler::get_binary_parser (enum token_t t)
{
        return this->rules[t].binary_parser;
}

Parser Compiler::get_unary_parser (enum token_t t)
{
        return this->rules[t].unary_parser;
}

enum Precedence Compiler::get_binary_precedence (enum token_t t)
{
        return this->rules[t].binary_prec;
}
enum Precedence Compiler::get_unary_precedence (enum token_t t)
{
        return this->rules[t].unary_prec;
}

bool Compiler::match (enum token_t t)
{
        if (this->curr_token.type == t) {
                this->advance ();
                return true;
        }

        return false;
}

void Compiler::advance ()
{
        if (this->at_end ())
                return;

        this->prev_token = this->curr_token;
        this->curr_token = this->scanner->scan_token ();
}

bool Compiler::at_end ()
{
        return this->curr_token.type == END;
}

enum token_t Compiler::peek ()
{
        return this->curr_token.type;
}

struct token Compiler::peek_token ()
{
        return this->curr_token;
}

enum token_t Compiler::previous ()
{
        return this->prev_token.type;
}

void Compiler::parse_statement ()
{
        switch (this->peek ()) {
        case LBRACE: this->parse_block (); break;
        case IF: this->parse_condition (); break;
        case WHILE: this->parse_while (); break;
        case FOR: this->parse_for (); break;
        case FUNC: this->parse_function_statement (); break;
        default: this->parse_expression_statement (); break;
        }
}

void Compiler::parse_expression_statement ()
{
        this->parse_expression ();
}

void Compiler::parse_expression ()
{
        this->parse_precedence (PREC_ASSIGNMENT);
}

void Compiler::parse_function_statement ()
{
        this->consume (FUNC, "expected func keyword");

        struct token func_name = this->peek_token ();

        this->consume (IDENTIFIER, "expected function name after func keyword");

        if (this->symbols->is_declared (func_name.name, func_name.len)) {
                this->parse_error ("function already defined", func_name);
        }

        this->consume (LPAREN, "expected '(' for function argument list");

        this->symbols->declare_local_variable (func_name.name, func_name.len);

        this->symbols = this->symbols->new_scope ();

        struct token args_list[255];
        int args_idx = 0;

        do {
                struct token t = this->peek_token ();

                if (!this->match (IDENTIFIER))
                        break;

                args_list[args_idx++] = t;
        } while (this->match (COMMA));

        for (int i = args_idx - 1; i >= 0; i--) {
                this->symbols->declare_function_parameter (args_list[i].name, args_list[i].len);
        }

        this->consume (RPAREN, "expected ')' after function argument list");
        this->consume (LBRACE, "expected '{' for function body");

        Function *old_function = this->function;
        this->function = new Function (func_name.name, func_name.len);

        this->symbol_to_function[this->convert_to_string (func_name.name, func_name.len)] = this->function;



        while (!this->match (RBRACE)) {
                this->parse_statement ();
        }

        int var_count = this->symbols->get_locals_count ();

        while (var_count-- > 0) {
                this->function->bytecode->emit_op (OPPOP);
        }

        this->function->bytecode->emit_op (OPRET);
        this->functions.push_back (this->function);
        this->function = old_function;
        this->symbols = this->symbols->pop_scope ();
}

void Compiler::parse_primary ()
{
        struct token token = this->peek_token ();

        if (this->match (IDENTIFIER)) {
                if (this->match (EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (EQUAL));
                        int32_t offset = this->symbols->get_stack_offset (token.name, token.len);

                        if (this->symbols->is_declared (token.name, token.len)) {
                                this->function->bytecode->emit_op (OPSTORE);
                                this->function->bytecode->write_int32 (offset);
                        } else {
                                this->symbols->declare_local_variable (token.name, token.len);
                        }

                        return;
                }

                if (!this->symbols->is_declared (token.name, token.len)) {
                        char variable_name[token.len + 1] = { '\0' };
                        strncpy (variable_name, token.name, token.len);
                        this->parse_error ("undeclared variable %s\n", token, variable_name);
                        return;
                }

                int32_t offset = this->symbols->get_stack_offset (token.name, token.len);

                if (this->match (PLUS_EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (PLUS_EQUAL));
                        this->function->bytecode->emit_op (OPLOAD);
                        this->function->bytecode->write_int32 (offset);
                        this->function->bytecode->emit_op (OPADD);
                        this->function->bytecode->emit_op (OPSTORE);
                        this->function->bytecode->write_int32 (offset);
                } else if (this->match (MINUS_EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (MINUS_EQUAL));
                        this->function->bytecode->emit_op (OPNEG);
                        this->function->bytecode->emit_op (OPLOAD);
                        this->function->bytecode->write_int32 (offset);
                        this->function->bytecode->emit_op (OPADD);
                        this->function->bytecode->emit_op (OPSTORE);
                        this->function->bytecode->write_int32 (offset);
                } else if (this->match (MULT_EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (MULT_EQUAL));
                        this->function->bytecode->emit_op (OPLOAD);
                        this->function->bytecode->write_int32 (offset);
                        this->function->bytecode->emit_op (OPMULT);
                        this->function->bytecode->emit_op (OPSTORE);
                        this->function->bytecode->write_int32 (offset);
                } else if (this->match (LPAREN)) {
                        int param_count = 0;
                        do {
                                this->parse_expression ();
                                param_count++;
                        } while (this->match (COMMA));

                        this->consume (RPAREN, "expected ')' after function call");

                        this->function->bytecode->emit_op (OPCALL);
                        this->function->bytecode->write_int32 (
                                this->resolve_function_placeholder (token.name, token.len));
                        while (param_count-- > 0) {
                                this->function->bytecode->emit_op (OPPOP);
                        }
                } else {
                        this->function->bytecode->emit_op (OPLOAD);
                        this->function->bytecode->write_int32 (offset);
                }
        } else if (this->match (INT)) {
                this->function->bytecode->emit_op (OPPUSH);
                this->function->bytecode->write_int32 (token.i);
        } else if (this->match (LPAREN)) {
                this->parse_precedence (PREC_ASSIGNMENT);
                this->match (RPAREN);
        }
}

void Compiler::parse_precedence (enum Precedence prec)
{
        enum token_t prefix = this->peek ();
        struct token prefix_token = this->peek_token ();
        Parser prefix_parser = this->get_unary_parser (prefix);

        if (!prefix_parser) {
                return;
                // this->parse_error ("cannot use %s in expression", prefix_token, prefix_token.name);
        }

        PARSER_FN (prefix_parser) ();

        while (prec <= this->get_binary_precedence (this->peek ())) {
                enum token_t binary = this->peek ();

                Parser binary_parser = this->get_binary_parser (binary);

                if (!binary_parser) {
                        char binary_op[prefix_token.len + 1] = { '\0' };
                        strncpy (binary_op, prefix_token.name, prefix_token.len);
                        this->parse_error ("unknown binary operation '%s'", this->peek_token (), binary_op);
                        exit (EXIT_FAILURE);
                }

                PARSER_FN (binary_parser) ();
        }
}

void Compiler::parse_logical ()
{
        enum token_t op = this->peek ();
        struct token op_token = this->peek_token ();
        if (!this->match (AND) && !this->match (OR)) {
                this->parse_error ("expected '&&' or '||' operation", op_token);
                return;
        }
        if (this->previous () == AND) {
                this->parse_precedence (PRECEDENCE_GREATER_THAN (AND));
        } else {
                this->parse_precedence (PRECEDENCE_GREATER_THAN (OR));
        }

        switch (op) {
        case AND: this->function->bytecode->emit_op (OPAND); break;
        case OR: this->function->bytecode->emit_op (OPOR); break;
        default: break;
        }
}

void Compiler::consume (enum token_t t, const char *error_message, ...)
{
        if (this->match (t))
                return;

        va_list args;
        va_start (args, error_message);
        fprintf (stderr, "[error on line %lu:%lu] ", this->curr_token.line, this->curr_token.col);
        vfprintf (stderr, error_message, args);
        va_end (args);
        fputc ('\n', stderr);
        this->highlight_line (this->peek_token ());
        exit (EXIT_FAILURE);
}

void Compiler::parse_error (const char *error, struct token t, ...)
{
        this->has_error = true;
        va_list args;
        va_start (args, t);
        fprintf (stderr, "[error on line %lu:%lu] ", t.line, t.col);
        vfprintf (stderr, error, args);
        va_end (args);
        this->highlight_line (t);
        exit (EXIT_FAILURE);
}

void Compiler::parse_block ()
{
        this->consume (LBRACE, "expected '{' after ");
        this->push_block_scope ();
        while (!this->match (RBRACE)) {
                this->parse_statement ();
        }
        this->pop_block_scope ();
}

void Compiler::parse_unary ()
{
        if (this->match (MINUS)) {
                this->parse_precedence (this->get_unary_precedence (MINUS));
                this->function->bytecode->emit_op (OPNEG);
        }
}

void Compiler::parse_comparison ()
{
        enum token_t op = this->peek ();
        struct token op_token = this->peek_token ();
        if (!this->match (GT) && !this->match (GTEQUAL) && !this->match (LT) && !this->match (LTEQUAL)) {
                this->parse_error ("expected >, >=, <=, < after lvalue", op_token);
                return;
        }

        this->parse_precedence (PRECEDENCE_GREATER_THAN (this->previous ()));

        switch (op) {
        case GT: this->function->bytecode->emit_op (OPGT); break;
        case GTEQUAL: this->function->bytecode->emit_op (OPGTEQ); break;
        case LT: this->function->bytecode->emit_op (OPLT); break;
        case LTEQUAL: this->function->bytecode->emit_op (OPLTEQ); break;
        default: break;
        }
}

void Compiler::parse_products ()
{
        enum token_t op = this->peek ();
        struct token op_token = this->peek_token ();
        if (!this->match (MULT) && !this->match (DIV)) {
                this->parse_error ("expected * or /", op_token);
                return;
        }

        this->parse_precedence (PRECEDENCE_GREATER_THAN (op));

        switch (op) {
        case MULT: this->function->bytecode->emit_op (OPMULT); break;
        case DIV: this->function->bytecode->emit_op (OPDIV); break;
        default: break;
        }
}

void Compiler::parse_terms ()
{
        enum token_t op = this->peek ();
        struct token op_token = this->peek_token ();
        if (!this->match (PLUS) && !this->match (MINUS)) {
                this->parse_error ("expected + or - operator", op_token);
                return;
        }

        this->parse_precedence (PRECEDENCE_GREATER_THAN (op));

        switch (op) {
        case PLUS: this->function->bytecode->emit_op (OPADD); break;
        case MINUS:
                this->function->bytecode->emit_op (OPNEG);
                this->function->bytecode->emit_op (OPADD);
                break;
        default: break;
        }
}

void Compiler::push_block_scope ()
{
        this->symbols = this->symbols->new_scope ();
}

void Compiler::pop_block_scope ()
{
        int symbol_count = this->symbols->get_locals_count ();

        for (int i = 0; i < symbol_count; i++) {
                this->function->bytecode->emit_op (OPPOP);
        }

        this->symbols = this->symbols->pop_scope ();
}

void Compiler::parse_condition ()
{
        this->consume (IF, "expected 'if' keyword");

        this->consume (LPAREN, "expected '(' after if keyword");

        // parse if condition
        this->parse_expression ();

        this->consume (RPAREN, "expected ')' after if condition");

        size_t offset_false = this->function->bytecode->emit_jump_false ();

        // parse if body
        this->parse_statement ();

        size_t offset_true = this->function->bytecode->emit_jump ();

        this->function->bytecode->patch_jump (offset_false);

        // check if there is an else clause, parse if so..
        if (this->match (ELSE)) {
                this->parse_statement ();
        }

        this->function->bytecode->patch_jump (offset_true);
}

void Compiler::parse_while ()
{
        this->consume (WHILE, "expected while keyword");
        this->consume (LPAREN, "expected '(' after while keyword");

        size_t start_offset = this->function->bytecode->address ();

        this->parse_expression ();

        this->consume (RPAREN, "expected ')' after while condition");

        size_t offset_false = this->function->bytecode->emit_jump_false ();

        this->parse_statement ();

        this->function->bytecode->emit_jump (start_offset);

        this->function->bytecode->patch_jump (offset_false);
}

void Compiler::parse_for ()
{
        this->consume (FOR, "expected for statement");

        this->consume (LPAREN, "expected '(' after for keyword");

        this->parse_expression ();
        this->consume (SEMICOLON, "expected ';' after for initializer");

        size_t start_offset = this->function->bytecode->address ();

        this->parse_expression ();
        this->consume (SEMICOLON, "expected ';' after for condition");
        size_t condition_false_offset = this->function->bytecode->emit_jump_false ();
        size_t condition_true_offset = this->function->bytecode->emit_jump ();

        size_t update_offset = this->function->bytecode->address ();
        this->parse_expression ();
        this->function->bytecode->emit_jump (start_offset);

        this->consume (RPAREN, "expected ')' after for statement");

        this->function->bytecode->patch_jump (condition_true_offset);
        this->parse_statement ();
        this->function->bytecode->emit_jump (update_offset);
        this->function->bytecode->patch_jump (condition_false_offset);
}

std::string Compiler::convert_to_string (char *s, size_t len)
{
        char buf[len + 1] = { '\0' };

        strncpy (buf, s, len);

        return std::string (buf);
}

int32_t Compiler::resolve_function_placeholder (char *func_name, size_t len)
{
        this->call_placeholders[this->next_placeholder_value++] = this->convert_to_string (func_name, len);

        return this->next_placeholder_value - 1;
}

Function *Compiler::resolve_placeholder (int32_t placeholder)
{
        std::string func_name = this->call_placeholders[placeholder];
        return this->symbol_to_function[func_name];
}

Bytecode *Compiler::link ()
{
        Bytecode *compiled_code = new Bytecode ();

        for (int i = 0; i < this->functions.size (); i++) {
                Function *f = this->functions[i];

                size_t entry_address = compiled_code->address ();

                f->set_entry_address (entry_address);

                compiled_code->import (f->bytecode->chunk, f->bytecode->count);
        }

        this->function->set_entry_address (compiled_code->address ());

        compiled_code->import (this->function->bytecode->chunk, this->function->bytecode->count);

        size_t c = 0;
        int32_t arg;
        enum OpCode op;

        while (compiled_code->instruction_at (&c, &op, &arg)) {
                if (op == OPCALL) {
                        *((int32_t *)&compiled_code->chunk[c - sizeof (int32_t)]) =
                                (int32_t)this->resolve_placeholder (arg)->entry_address;
                }
        }

        return compiled_code;
}

Bytecode *Compiler::compile ()
{
        while (!this->match (END)) {
                this->parse_statement ();
        }

        this->function->bytecode->emit_op (OPHALT);

        if (this->scanner->has_errors)
                return NULL;

        if (this->has_error)
                return NULL;

        return this->link ();
}