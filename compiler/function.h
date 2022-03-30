#ifndef function_h
#define function_h

#include "bytecode.h"
#include "symbols.h"
#include "scanner.h"

class Function {

    public:

        Function(char *name, size_t len, struct token f);
        Bytecode *bytecode;
        char *name;
        struct token f;
        size_t len;
        size_t entry_address;
        void set_entry_address(size_t address);
    
};
#endif