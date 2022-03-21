#ifndef function_h
#define function_h

#include "bytecode.h"
#include "symbols.h"

class Function {

    public:

        Function(char *name, size_t len);
        Bytecode *bytecode;
        char *name;
        size_t len;
        size_t entry_address;
        void set_entry_address(size_t address);
    
};
#endif