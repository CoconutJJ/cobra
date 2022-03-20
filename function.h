#ifndef function_h
#define function_h

#include "bytecode.h"
#include "symbols.h"

class Function {

    public:
        Function(char *name);
        Bytecode *bytecode;
        char *name;
        size_t entry_address;
        void set_entry_address(size_t address);

};
#endif