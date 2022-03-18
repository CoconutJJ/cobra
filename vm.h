#ifndef vm_h
#define vm_h
#include "bytecode.h"

class VM {
        // public:

    private:
        uint8_t *bytecode;
        uint8_t stack[1024];

        uint8_t *sp;
        uint8_t *bp;
        uint8_t *ip;

        uint8_t read_uint8 ();
        uint16_t read_uint16 ();
        uint32_t read_uint32 ();
        uint64_t read_uint64 ();

        enum OpCode read_op ();

        void push_uint64 (uint64_t i);
        void push_uint32 (uint32_t i);
        void push_uint16 (uint16_t i);
        void push_uint8 (uint8_t i);

        uint8_t pop_uint8 ();
        uint16_t pop_uint16 ();
        uint32_t pop_uint32 ();
        uint64_t pop_uint64 ();

        
};

#endif