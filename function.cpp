#include "function.h"
#include <string.h>

Function::Function (char *name)
{
        this->bytecode = new Bytecode ();
        
        this->name = (char *)calloc (strlen (name) + 1, sizeof (char));

        if (!this->name) {
            perror("calloc");
            exit(EXIT_FAILURE);
        }

        strcpy(this->name, name);

}

void Function::set_entry_address (size_t address)
{
        this->bytecode->set_address_offset (address);
}
