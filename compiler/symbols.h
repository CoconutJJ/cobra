#ifndef symbols_h
#define symbols_h

#include <string>
#include <unordered_map>
class Symbols {
    public:
        int32_t scope_level;
        int32_t local_offset;
        int32_t param_offset;
        int32_t locals_count;
        Symbols *next_scope;
        Symbols *prev_scope;
        std::unordered_map<std::string, int32_t> offsets{};
        std::unordered_map<std::string, size_t> sizes{};
        Symbols (Symbols *prev_scope, size_t local_offset, long scope_level);
        int32_t get_stack_offset (char *variable_name, size_t len);
        bool declare_local_variable (char *variable_name, size_t len);
        bool declare_function_parameter (char *parameter_name, size_t len);
        bool has_symbol (char *symbol, size_t len);
        bool is_declared (char *variable_name, size_t len);
        int get_locals_count();
        Symbols *new_scope ();
        Symbols *pop_scope ();

    private:
        std::string convert_to_string (char *s, size_t len);
};
#endif