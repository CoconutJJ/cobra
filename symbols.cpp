#include "symbols.h"
#include <string.h>
#include <string>

Symbols::Symbols (Symbols *prev_scope, size_t local_offset, long scope_level)
{
        this->local_offset = local_offset;
        this->param_offset = 0;
        this->scope_level = scope_level;
        this->next_scope = NULL;
        this->prev_scope = prev_scope;
        this->offsets = std::unordered_map<std::string, long>{};
}

int Symbols::get_stack_offset (char *variable_name, size_t len)
{
        if (this->has_symbol (variable_name, len)) {
                std::string s = this->convert_to_string (variable_name, len);
                return this->offsets[s];
        }

        if (!this->prev_scope)
                return -1;

        return this->prev_scope->get_stack_offset (variable_name, len);
}

bool Symbols::declare_local_variable (char *variable_name, size_t len)
{
        std::string s = this->convert_to_string (variable_name, len);

        this->offsets[s] = this->local_offset;
        this->local_offset += 1;
        this->symbols_count++;
        return true;
}

bool Symbols::declare_function_parameter (char *variable_name, size_t len)
{
        std::string s = this->convert_to_string (variable_name, len);

        this->param_offset += 1;
        this->symbols_count++;
        this->offsets[s] = -this->param_offset;

        return true;
}

std::string Symbols::convert_to_string (char *str, size_t len)
{
        char *buf = (char *)malloc ((len + 1) * sizeof (char));

        if (!buf) {
                perror ("malloc");
                exit (EXIT_FAILURE);
        }

        strncpy (buf, str, len);
        buf[len] = '\0';
        std::string s (buf);

        return s;
}

bool Symbols::is_declared (char *variable_name, size_t len) {

        if (this->has_symbol(variable_name, len)) return true;

        if (!this->prev_scope) return false;

        return this->prev_scope->is_declared(variable_name, len);

}

bool Symbols::has_symbol (char *symbol, size_t len)
{
        std::string s = this->convert_to_string (symbol, len);

        return this->offsets.find (s) != this->offsets.end ();
}

int Symbols::get_symbols_count ()
{
        return this->symbols_count;
}

Symbols *Symbols::new_scope ()
{
        this->next_scope = new Symbols (this, this->local_offset, this->scope_level + 1);

        return this->next_scope;
}

Symbols *Symbols::pop_scope ()
{
        Symbols *prev_scope = this->prev_scope;

        delete this;

        return prev_scope;
}
