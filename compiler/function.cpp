#include "function.h"
#include "scanner.h"
#include <string.h>

Function::Function (char *name, size_t len, struct token f)
{
        this->bytecode = new Bytecode ();
        this->f = f;
        this->name = name;
        this->len = len;
}

void Function::set_entry_address (size_t address)
{
        this->entry_address = address;
        this->bytecode->set_address_offset (address);
}
