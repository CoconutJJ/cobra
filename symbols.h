#ifndef symbols_h
#define symbols_h

#include <string>
#include <unordered_map>
class Symbols {
    public:
        long scope_level;
        long local_offset;
        long param_offset;
        Symbols *next_scope;
        Symbols *prev_scope;
        std::unordered_map<std::string, long> offsets;
        std::unordered_map<std::string, size_t> sizes;
        Symbols (Symbols *prev_scope, size_t local_offset, long scope_level);
        int get_stack_offset (char *variable_name, size_t len);
        size_t get_local_size (char *variable_name, size_t len);
        bool declare_local_variable (char *variable_name, size_t len, size_t variable_size);
        bool declare_function_parameter (char *parameter_name, size_t len, size_t parameter_size);
        bool has_symbol (char *symbol, size_t len);
        Symbols *new_scope ();
        Symbols *pop_scope ();

    private:
        std::string convert_to_string (char *s, size_t len);
};
#endif