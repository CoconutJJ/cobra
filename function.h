#ifndef function_h
#define function_h

#include "bytecode.h"
#include "symbols.h"

class Function {

    public:
        Function(char *name);
        Bytecode *bytecode;
        char *name;

        void set_entry_address(size_t address);

};
#endif