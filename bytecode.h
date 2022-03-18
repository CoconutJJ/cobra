#ifndef bytecode_h
#define bytecode_h

#include <stdint.h>
#include <stdlib.h>

enum OpCode {
    OPADD,
    OPNEG,
    OPMULT,
    OPDIV,
    OPMOD,
    OPEQ,
    OPGT,
    OPLT,
    OPGTEQ,
    OPLTEQ,
    OPAND,
    OPOR,
    OPJMP,
    OPJMPFALSE,
    OPSTORE,
    OPLOAD,
    OPPUSH,
    OPPOP,
    OPCALL,
    OPHALT,
    OPBP
};

class Bytecode {
    public:
        uint8_t *chunk;
        size_t count;
        size_t capacity;
        Bytecode();
        void emit_op(enum OpCode op);
        void patch_jump(size_t offset);
        size_t emit_jump(uint32_t address = 0xFFFFFFFF);
        size_t emit_jump_false();
        size_t address();
        size_t write_uint8(uint8_t sh);
        size_t write_uint16(uint16_t sh);
        size_t write_uint32(uint32_t sh);
        size_t write_uint64(uint64_t sh);
        
    private:
        void resize_chunk(size_t min_size);

};
#endif