#include "function.h"
#include <string.h>

Function::Function (char *name, size_t len)
{
        this->bytecode = new Bytecode ();
        
        this->name = name;
        this->len = len;
        if (!this->name) {
            perror("calloc");
            exit(EXIT_FAILURE);
        }

}

void Function::set_entry_address (size_t address)
{
        this->entry_address = address;
        this->bytecode->set_address_offset (address);
}
