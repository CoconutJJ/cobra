#include "compiler.h"
#include "scanner.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

Compiler::Compiler (char *src_code)
{
#define RULE(token, binary, bprec, unary, uprec)                                                                       \
        this->rules[token] = {                                                                                         \
                .binary_parser = (binary),                                                                             \
                .unary_parser = (unary),                                                                               \
                .binary_prec = (bprec),                                                                                \
                .unary_prec = (uprec)                                                                                  \
        }

        this->scanner = new Scanner (src_code);
        this->bytecode = new Bytecode ();
        RULE (PLUS, &Compiler::parse_terms, PREC_TERM, NULL, PREC_NONE);
        RULE (MINUS, &Compiler::parse_terms, PREC_TERM, &Compiler::parse_unary, PREC_UNARY);
        RULE (MULT, &Compiler::parse_products, PREC_PRODUCT, NULL, PREC_NONE);
        RULE (DIV, &Compiler::parse_products, PREC_PRODUCT, NULL, PREC_PRODUCT);
        RULE (LPAREN, NULL, PREC_NONE, &Compiler::parse_primary, PREC_PRIMARY);
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
        return Compiler::curr_token.type;
}

void Compiler::parse_primary ()
{
        struct token token = this->peekToken ();

        if (this->match (IDENTIFIER)) {
                size_t local_size = this->symbols->get_local_size (token.name, token.len);
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
                        this->bytecode->write_uint32 (local_size);
                        this->bytecode->write_uint32 (offset);

                } else if (this->match (PLUS_EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (PLUS_EQUAL));

                        this->bytecode->emit_op (OPLOAD);
                        this->bytecode->write_uint32 (local_size);
                        this->bytecode->write_uint32 (offset);

                        this->bytecode->emit_op (OPADD);
                        this->bytecode->write_uint32 (local_size);

                        this->bytecode->emit_op (OPSTORE);
                        this->bytecode->write_uint32 (local_size);
                        this->bytecode->write_uint32 (offset);
                } else if (this->match (MINUS_EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (PLUS_EQUAL));
                        this->bytecode->emit_op (OPNEG);
                        this->bytecode->write_uint32 (local_size);

                        this->bytecode->emit_op (OPLOAD);
                        this->bytecode->write_uint32 (local_size);
                        this->bytecode->write_uint32 (offset);

                        this->bytecode->emit_op (OPADD);
                        this->bytecode->write_uint32 (local_size);

                        this->bytecode->emit_op (OPSTORE);
                        this->bytecode->write_uint32 (local_size);
                        this->bytecode->write_uint32 (offset);
                } else if (this->match (MULT_EQUAL)) {
                        this->parse_precedence (PRECEDENCE_GREATER_THAN (MULT_EQUAL));

                        this->bytecode->emit_op (OPLOAD);
                        this->bytecode->write_uint32 (local_size);
                        this->bytecode->write_uint32 (offset);

                        this->bytecode->emit_op (OPMULT);
                        this->bytecode->write_uint32 (local_size);

                        this->bytecode->emit_op (OPSTORE);
                        this->bytecode->write_uint32 (local_size);
                        this->bytecode->write_uint32 (offset);
                } else {
                        this->bytecode->emit_op (OPLOAD);
                        this->bytecode->write_uint32 (local_size);
                        this->bytecode->write_uint32 (offset);
                }

        } else if (this->match (INT) || this->match (FLOAT) || this->match (DOUBLE)) {
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

void Compiler::consume (enum token_t t, char *error_message, ...)
{
        if (!this->match (t)) {
                va_list args;
                va_start (args, error_message);
                fprintf (stderr, "[error on line %d:%d] ", this->curr_token.line, this->curr_token.col);
                vfprintf (stderr, error_message, args);
                va_end (args);
        }
}

void Compiler::parse_error (char *error, ...)
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

void Compiler::parse_terms ()
{
        enum token_t op = this->peek ();

        if (!this->match (PLUS) && !this->match (MINUS)) {
                this->parse_error ("expected + or - operator");
                return;
        }

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
