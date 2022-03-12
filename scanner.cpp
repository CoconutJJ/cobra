#include "scanner.h"
#include "string.h"

Scanner::Scanner(char * src_code) {
        this->source = src_code;
}

void Scanner::advance ()
{
        this->curr++;
        this->col_no++;
}

bool Scanner::match (char c)
{
        if (*this->curr == c) {
                advance ();
                return true;
        }
        return false;
}

bool Scanner::at_end ()
{
        return *this->curr == '\0';
}

char Scanner::peek ()
{
        return *this->curr;
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
        return '0' <= c && c <= '9';
}

bool Scanner::is_valid_identifier_char (char c)
{
        return this->is_alpha (c) || this->is_numeric (c) || c == '_';
}

enum token_t Scanner::match_keyword (char *keyword, int length)
{
        if (strncmp (keyword, "if", length) == 0)
                return IF;
        if (strncmp (keyword, "while", length) == 0)
                return WHILE;
        if (strncmp (keyword, "for", length) == 0)
                return FOR;
        if (strncmp (keyword, "return", length) == 0)
                return RETURN;
        if (strncmp (keyword, "func", length) == 0)
                return FUNC;
                
        return IDENTIFIER;
}

struct token Scanner::match_identifier ()
{
        char *start = this->curr - 1;

        while (this->is_valid_identifier_char (this->peek ()) && !this->at_end ())
                this->advance ();

        struct token t;

        t.name = start;
        t.len = this->curr - start;
        t.line = this->line_no;
        t.col = this->col_no;
        t.type = this->match_keyword (start, t.len);
}

struct token Scanner::match_number ()
{
        char *start = this->curr - 1;

        struct token t;

        while (this->is_numeric (this->peek ()) && !this->at_end()) {
                this->advance ();
        }

        if (this->match ('.')) {
                t.type = DOUBLE;
                while (this->is_numeric (this->peek ()) && !this->at_end()) {
                        this->advance ();
                }

                t.d = strtod (start, NULL);
        } else {
                t.type = INT;
                t.i = strtol (start, NULL, 10);
        }

        return t;
}

struct token Scanner::scan_token ()
{
        struct token t;

        for (;;) {
                char c = this->peek ();
                this->advance ();
                t.line = this->line_no;
                t.col = this->col_no;
                switch (c) {
                case '+': t.type = PLUS; break;
                case '-': t.type = MINUS; break;
                case '*': t.type = MULT; break;
                case '/': {
                        if (this->match ('/')) {
                                this->skip_comment ();
                                continue;
                        }
                        t.type = DIV;
                        break;
                }
                case '%': t.type = MOD_EQUAL; break;
                case '(': t.type = LPAREN; break;
                case ')': t.type = RPAREN; break;
                case '[': t.type = LBRACKET; break;
                case ']': t.type = RBRACKET; break;
                case '{': t.type = LBRACE; break;
                case '}': t.type = RBRACE; break;
                case '|': t.type = this->match ('|') ? OR : BIT_OR; break;
                case '>': t.type = this->match ('=') ? GTEQUAL : GT; break;
                case '<': t.type = this->match ('=') ? LTEQUAL : LT; break;
                case '=': t.type = this->match ('=') ? EQUAL_EQUAL : EQUAL; break;
                case '!': t.type = this->match ('=') ? BANG_EQUAL : BANG; break;
                case '~': t.type = BIT_NOT; break;
                case '&': t.type = this->match ('&') ? AND : BIT_AND; break;
                case ' ': continue;
                case '\n': {
                        this->line_no++;
                        this->col_no = 0;
                        continue;
                }
                default: {
                        if (this->is_alpha (c)) {
                                return this->match_identifier ();
                        } else if (this->is_numeric (c)) {
                                return this->match_number ();
                        }

                        break;
                }
                }
                return t;
        }
}