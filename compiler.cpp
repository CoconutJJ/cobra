#include "compiler.h"
#include "scanner.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

Compiler::Compiler (char *src_code)
{
        this->setup (src_code);
}

Compiler::Compiler (FILE *source_fp)
{
        size_t capacity = 8;
        size_t read_size = 8;

        char *source_buf = (char *)malloc (capacity * sizeof (char));
        if (!source_buf)
                throw -1;

        char *curr = source_buf;

        while (fread (curr, 1, read_size, source_fp) == read_size) {
                curr = source_buf + capacity;

                read_size = capacity;

                capacity *= 2;

                source_buf = (char *)realloc (source_buf, capacity * sizeof (char));

                if (!source_buf)
                        throw -1;
        }

        this->setup (source_buf);
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
        this->has_error = false;
        this->scanner = new Scanner (src_code);
        this->bytecode = new Bytecode ();
        this->advance ();

        RULE (PLUS, &Compiler::parse_terms, PREC_TERM, NULL, PREC_NONE);
        RULE (MINUS, &Compiler::parse_terms, PREC_TERM, &Compiler::parse_unary, PREC_UNARY);
        RULE (MULT, &Compiler::parse_products, PREC_PRODUCT, NULL, PREC_NONE);
        RULE (DIV, &Compiler::parse_products, PREC_PRODUCT, NULL, PREC_PRODUCT);
        RULE (LPAREN, NULL, PREC_NONE, &Compiler::parse_primary, PREC_PRIMARY);
        RULE (INT, NULL, PREC_PRIMARY, &Compiler::parse_primary, PREC_PRIMARY);
        RULE (FLOAT, NULL, PREC_NONE, &Compiler::parse_unary, PREC_PRIMARY);
        RULE (END, NULL, PREC_NONE, NULL, PREC_NONE);
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
        this->prev_token = this->curr_token;
        this->curr_token = this->scanner->scan_token ();
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
        default: this->parse_expression (); break;
        }
}

void Compiler::parse_expression_statement ()
{
        this->parse_expression ();
        this->bytecode->emit_op (OPPOP);
}

void Compiler::parse_expression ()
{
        this->parse_precedence (PREC_ASSIGNMENT);
}

void Compiler::parse_primary ()
{
        struct token token = this->peek_token ();

        if (this->match (IDENTIFIER)) {
                int offset = this->symbols->get_stack_offset (token.name, token.len);

                char variable_name[token.len + 1] = { '\0' };
                strncpy (variable_name, token.name, token.len);

                if (offset == -1) {
                        this->parse_error ("undeclared variable %s", variable_name);
                        return;
                }

                if (this->match (EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (EQUAL));
                        this->bytecode->emit_op (OPSTORE);
                        this->bytecode->write_uint32 (offset);
                } else if (this->match (PLUS_EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (PLUS_EQUAL));
                        this->bytecode->emit_op (OPLOAD);
                        this->bytecode->write_uint32 (offset);
                        this->bytecode->emit_op (OPADD);
                        this->bytecode->emit_op (OPSTORE);
                        this->bytecode->write_uint32 (offset);
                } else if (this->match (MINUS_EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (MINUS_EQUAL));
                        this->bytecode->emit_op (OPNEG);
                        this->bytecode->emit_op (OPLOAD);
                        this->bytecode->write_uint32 (offset);
                        this->bytecode->emit_op (OPADD);
                        this->bytecode->emit_op (OPSTORE);
                        this->bytecode->write_uint32 (offset);
                } else if (this->match (MULT_EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (MULT_EQUAL));
                        this->bytecode->emit_op (OPLOAD);
                        this->bytecode->write_uint32 (offset);
                        this->bytecode->emit_op (OPMULT);
                        this->bytecode->emit_op (OPSTORE);
                        this->bytecode->write_uint32 (offset);
                } else {
                        this->bytecode->emit_op (OPLOAD);
                        this->bytecode->write_uint32 (offset);
                }
        } else if (this->match (INT)) {
                this->bytecode->emit_op (OPPUSH);
                this->bytecode->write_uint32 (token.i);
        } else if (this->match (LPAREN)) {
                this->parse_precedence (PREC_ASSIGNMENT);
                this->match (RPAREN);
        }
}

void Compiler::parse_precedence (enum Precedence prec)
{
        enum token_t prefix = this->peek ();
        this->advance ();
        Parser prefix_parser = this->get_unary_parser (prefix);

        PARSER_FN (prefix_parser) ();

        while (prec <= this->get_binary_precedence (this->peek ())) {
                enum token_t binary = this->peek ();
                Parser binary_parser = this->get_binary_parser (binary);
                PARSER_FN (binary_parser) ();
        }
}

void Compiler::parse_logical ()
{
        enum token_t op = this->peek ();

        if (!this->match (AND) && !this->match (OR)) {
                this->parse_error ("expected '&&' or '||' operation");
                return;
        }
        if (this->previous () == AND) {
                this->parse_precedence (PRECEDENCE_GREATER_THAN (AND));
        } else {
                this->parse_precedence (PRECEDENCE_GREATER_THAN (OR));
        }

        switch (op) {
        case AND: this->bytecode->emit_op (OPAND); break;
        case OR: this->bytecode->emit_op (OPOR); break;
        default: break;
        }
}

void Compiler::consume (enum token_t t, const char *error_message, ...)
{
        if (!this->match (t)) {
                va_list args;
                va_start (args, error_message);
                fprintf (stderr, "[error on line %d:%d] ", this->curr_token.line, this->curr_token.col);
                vfprintf (stderr, error_message, args);
                va_end (args);
        }
}

void Compiler::parse_error (const char *error, ...)
{
        this->has_error = true;
        va_list args;
        va_start (args, error);
        fprintf (stderr, "[error on line %d:%d] ", this->curr_token.line, this->curr_token.col);
        vfprintf (stderr, error, args);
        va_end (args);
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
                this->bytecode->emit_op (OPNEG);
        }
}

void Compiler::parse_comparison ()
{
        enum token_t op = this->peek ();

        if (!this->match (GT) && !this->match (GTEQUAL) && !this->match (LT) && !this->match (LTEQUAL)) {
                this->parse_error ("expected >, >=, <=, < after lvalue");
                return;
        }

        this->parse_precedence ((enum Precedence) ((int)this->get_binary_precedence (this->previous ()) + 1));

        switch (op) {
        case GT: this->bytecode->emit_op (OPGT); break;
        case GTEQUAL: this->bytecode->emit_op (OPGTEQ); break;
        case LT: this->bytecode->emit_op (OPLT); break;
        case LTEQUAL: this->bytecode->emit_op (OPLTEQ); break;
        default: break;
        }
}

void Compiler::parse_products ()
{
        enum token_t op = this->peek ();

        if (!this->match (MULT) && !this->match (DIV)) {
                this->parse_error ("expected * or /");
                return;
        }

        this->parse_precedence (PRECEDENCE_GREATER_THAN (op));

        switch (op) {
        case MULT: this->bytecode->emit_op (OPMULT); break;
        case DIV: this->bytecode->emit_op (OPDIV); break;
        default: break;
        }
}

void Compiler::parse_terms ()
{
        enum token_t op = this->peek ();

        if (!this->match (PLUS) && !this->match (MINUS)) {
                this->parse_error ("expected + or - operator");
                return;
        }

        this->parse_precedence (PRECEDENCE_GREATER_THAN (op));

        switch (op) {
        case PLUS: this->bytecode->emit_op (OPADD); break;
        case MINUS:
                this->bytecode->emit_op (OPNEG);
                this->bytecode->emit_op (OPADD);
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
        long offset = this->symbols->local_offset;
        this->bytecode->emit_op (OPPOP);
        this->bytecode->write_uint32 (offset);
        this->symbols = this->symbols->pop_scope ();
}

void Compiler::parse_condition ()
{
        if (!this->match (IF)) {
                this->parse_error ("expected if statement");
                return;
        }

        this->consume (LPAREN, "expected '(' after if keyword");

        this->parse_expression ();

        this->consume (RPAREN, "expected ')' after if condition");

        size_t offset_false = this->bytecode->emit_jump_false ();

        this->parse_statement ();

        size_t offset_true = this->bytecode->emit_jump ();

        this->bytecode->patch_jump (offset_false);

        if (this->match (ELSE)) {
                this->parse_statement ();
        }

        this->bytecode->patch_jump (offset_true);
}

void Compiler::parse_while ()
{
        if (!this->match (WHILE)) {
                this->parse_error ("expected while statement");
                return;
        }
        this->consume (LPAREN, "expected '(' after while keyword");

        size_t start_offset = this->bytecode->address ();

        this->parse_expression ();

        this->consume (RPAREN, "expected ')' after while condition");

        size_t offset_false = this->bytecode->emit_jump_false ();

        this->parse_statement ();

        this->bytecode->emit_jump (start_offset);

        this->bytecode->patch_jump (offset_false);
}

void Compiler::parse_for ()
{
        this->consume (FOR, "expected for statement");

        this->consume (LPAREN, "expected '(' after for keyword");

        this->parse_expression ();
        this->consume (SEMICOLON, "expected ';' after for initializer");

        size_t start_offset = this->bytecode->address ();

        this->parse_expression ();
        this->consume (SEMICOLON, "expected ';' after for condition");
        size_t condition_false_offset = this->bytecode->emit_jump_false ();
        size_t condition_true_offset = this->bytecode->emit_jump ();

        size_t update_offset = this->bytecode->address ();
        this->parse_expression ();
        this->consume (SEMICOLON, "expected ';' after for update");
        this->bytecode->emit_jump (start_offset);

        this->consume (RPAREN, "expected ')' after for statement");

        this->bytecode->patch_jump (condition_true_offset);
        this->parse_statement ();
        this->bytecode->emit_jump (update_offset);
        this->bytecode->patch_jump (condition_false_offset);
}

Bytecode *Compiler::compile ()
{
        while (!this->match (END)) {
                this->parse_statement ();
        }

        if (this->has_error)
                return NULL;

        return this->bytecode;
}