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
}

int Symbols::get_stack_offset (char *variable_name, size_t len)
{
        if (!this->has_symbol (variable_name, len))
                return -1;

        std::string s = this->convert_to_string (variable_name, len);

        return this->offsets[s];
}

bool Symbols::declare_local_variable (char *variable_name, size_t len, size_t variable_size)
{
        if (!this->has_symbol (variable_name, len))
                return false;

        std::string s = this->convert_to_string (variable_name, len);

        this->offsets[s] = this->local_offset;
        this->sizes[s] = variable_size;
        this->local_offset += variable_size;

        return true;
}

bool Symbols::declare_function_parameter (char *variable_name, size_t len, size_t parameter_size)
{
        if (!this->has_symbol (variable_name, len))
                return false;

        std::string s = this->convert_to_string (variable_name, len);

        this->param_offset += parameter_size;

        this->offsets[s] = -this->param_offset;

        return true;
}

std::string Symbols::convert_to_string (char *str, size_t len)
{
        char buf[len + 1] = { '\0' };
        strncpy (buf, str, len);

        std::string s (buf);

        return s;
}

bool Symbols::has_symbol (char *symbol, size_t len)
{
        std::string s = this->convert_to_string (symbol, len);

        return this->offsets.find (s) != this->offsets.end ();
}

size_t Symbols::get_local_size (char *variable_name, size_t len)
{
        
        if (!this->has_symbol(variable_name, len))
                return 0;
        
        std::string s = this->convert_to_string(variable_name, len);
        
        return this->sizes[s];
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
