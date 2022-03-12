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
    OPJMPTRUE,
    OPSTORE,
    OPLOAD,
    OPPUSH,
    OPPOP,
    OPCALL
};

class Bytecode {
    public:
        uint8_t *chunk;
        size_t count;
        size_t capacity;
        Bytecode();
        void emit_op(enum OpCode op);
        void patch_jump(size_t offset);
        size_t emit_jump();
        size_t write_uint8(uint8_t sh);
        size_t write_uint16(uint16_t sh);
        size_t write_uint32(uint32_t sh);
        
    private:
        void resize_chunk(size_t min_size);

};
#endif