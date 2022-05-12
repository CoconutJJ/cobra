#include "scanner.h"
#include "string.h"
#include <stdarg.h>
#include <stdio.h>
Scanner::Scanner (char *src_code)
{
        this->source = src_code;
        this->curr = this->source;
        this->line_no = 1;
        this->col_no = 0;
        this->curr_line = this->curr;
        this->has_errors = false;
}

void Scanner::advance ()
{
        if (this->at_end ())
                return;

        this->curr++;
        this->col_no++;
}

bool Scanner::match (char c)
{
        if (this->at_end ())
                return false;

        if (*(this->curr) == c) {
                this->advance ();
                return true;
        }
        return false;
}

bool Scanner::at_end ()
{
        return *(this->curr) == '\0';
}

char Scanner::peek ()
{
        if (this->at_end ())
                return '\0';

        return *(this->curr);
}

void Scanner::scan_error (const char *message, ...)
{
        this->has_errors = true;
        va_list args;
        va_start (args, message);
        fprintf (stderr, "[syntax error on line %d:%d] ", this->line_no, this->col_no);
        vfprintf (stderr, message, args);
        va_end (args);
}

void Scanner::skip_comment ()
{
        while (this->peek () != '\n' && !this->at_end ())
                this->advance ();
}

bool Scanner::is_alpha (char c)
{
        return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

bool Scanner::is_numeric (char c)
{
        return ('0' <= c && c <= '9');
}

bool Scanner::is_valid_identifier_char (char c)
{
        return this->is_alpha (c) || this->is_numeric (c) || c == '_';
}

enum token_t Scanner::match_keyword (char *keyword, int length)
{
#define MAX(a, b) ((a) < (b) ? (b) : (a))

        if (strncmp (keyword, "if", MAX (2, length)) == 0)
                return IF;
        if (strncmp (keyword, "else", MAX (4, length)) == 0)
                return ELSE;
        if (strncmp (keyword, "while", MAX (5, length)) == 0)
                return WHILE;
        if (strncmp (keyword, "for", MAX (3, length)) == 0)
                return FOR;
        if (strncmp (keyword, "return", MAX (6, length)) == 0)
                return RETURN;
        if (strncmp (keyword, "func", MAX (4, length)) == 0)
                return FUNC;

#undef MAX

        return IDENTIFIER;
}

struct token Scanner::match_identifier ()
{
        struct token t;

        t.col = this->col_no;

        char *start = this->curr - 1;

        while (this->is_valid_identifier_char (this->peek ()) && !this->at_end ())
                this->advance ();

        t.name = start;
        t.len = this->curr - start;
        t.line = this->line_no;
        t.type = this->match_keyword (start, t.len);
        t.code_line = this->curr_line;
        return t;
}

struct token Scanner::match_number ()
{
        char *start = this->curr - 1;

        struct token t;
        t.code_line = this->curr_line;
        while (this->is_numeric (this->peek ()) && !this->at_end ()) {
                this->advance ();
        }

        if (this->match ('.')) {
                t.type = DOUBLE;
                while (this->is_numeric (this->peek ()) && !this->at_end ()) {
                        this->advance ();
                }

                t.d = strtod (start, NULL);
        } else {
                t.type = INT;
                t.i = strtol (start, NULL, 10);
        }

        return t;
}

void Scanner::highlight_line (size_t start_col, size_t end_col)
{
        char *start = this->curr_line;
        size_t line_len = 0;

        fputs ("\t|\n", stderr);
        fprintf (stderr, " %d\t| ", this->line_no);

        while (*start && *start != '\n') {
                fputc (*start, stderr);
                start++;
                line_len++;
        }

        fputc ('\n', stderr);
        fputs ("\t| ", stderr);

        for (size_t i = 0; i < start_col; i++) {
                fputc (' ', stderr);
        }

        for (size_t i = start_col; i < line_len; i++)
                fputc (i < end_col ? '^' : '~', stderr);

        fputc ('\n', stderr);
}

struct token Scanner::scan_token ()
{
        struct token t;
        for (;;) {
                if (this->at_end ())
                        return (struct token){ .type = END };

                char c = this->peek ();
                t.name = this->curr;

                this->advance ();
                t.line = this->line_no;
                t.col = this->col_no;
                t.code_line = this->curr_line;
                switch (c) {
                case '+': t.type = this->match ('=') ? PLUS_EQUAL : PLUS; break;
                case '-': t.type = this->match ('=') ? MINUS_EQUAL : MINUS; break;
                case '*': t.type = this->match ('=') ? MULT_EQUAL : MULT; break;
                case '/': {
                        if (this->match ('/')) {
                                this->skip_comment ();
                                continue;
                        }
                        t.type = this->match ('=') ? DIV_EQUAL : DIV;
                        t.len = 1;
                        break;
                }
                case '%':
                        t.type = MOD_EQUAL;
                        t.len = 1;
                        break;
                case '(':
                        t.type = LPAREN;
                        t.len = 1;
                        break;
                case ')':
                        t.type = RPAREN;
                        t.len = 1;
                        break;
                case '[':
                        t.type = LBRACKET;
                        t.len = 1;
                        break;
                case ']':
                        t.type = RBRACKET;
                        t.len = 1;
                        break;
                case '{':
                        t.type = LBRACE;
                        t.len = 1;
                        break;
                case '}':
                        t.type = RBRACE;
                        t.len = 1;
                        break;
                case '|':
                        t.type = this->match ('|') ? OR : BIT_OR;
                        t.len = 2;
                        break;
                case '>':
                        t.type = this->match ('=') ? GTEQUAL : GT;
                        t.len = 2;
                        break;
                case '<':
                        t.type = this->match ('=') ? LTEQUAL : LT;
                        t.len = 2;
                        break;
                case '=':
                        t.type = this->match ('=') ? EQUAL_EQUAL : EQUAL;
                        t.len = 2;
                        break;
                case '!':
                        t.type = this->match ('=') ? BANG_EQUAL : BANG;
                        t.len = 2;
                        break;
                case '~':
                        t.type = BIT_NOT;
                        t.len = 1;
                        break;
                case '&':
                        t.type = this->match ('&') ? AND : BIT_AND;
                        t.len = 2;
                        break;
                case ';':
                        t.type = SEMICOLON;
                        t.len = 1;
                        break;
                case ',':
                        t.type = COMMA;
                        t.len = 1;
                        break;
                case '\t':
                case ' ': continue;
                case '\n': {
                        this->line_no++;
                        this->col_no = 0;
                        this->curr_line = this->curr;
                        continue;
                }
                default: {
                        if (this->is_alpha (c)) {
                                return this->match_identifier ();
                        } else if (this->is_numeric (c)) {
                                return this->match_number ();
                        } else {
                                this->scan_error ("unexpected symbol '%c'\n", c);
                                this->highlight_line (this->col_no - 1, this->col_no);
                                exit (EXIT_FAILURE);
                        }

                        break;
                }
                }
                return t;
        }
}