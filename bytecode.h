#ifndef bytecode_h
#define bytecode_h

#include <stdint.h>
#include <stdlib.h>

#define AS_INT32(ptr) (*((int32_t *)ptr))

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
    OPSTOREBP,
    OPPUSHBP,
    OPPUSHSP,
    OPSTORESP
};

class Bytecode {
    public:
        int8_t *chunk;
        size_t count;
        size_t capacity;
        size_t address_offset;
        Bytecode();
        void emit_op(enum OpCode op);
        void patch_jump(size_t offset);
        size_t emit_jump(int32_t address = 0xFFFFFFFF);
        size_t emit_jump_false();
        size_t address();
        size_t write_int8(int8_t sh);
        size_t write_int16(int16_t sh);
        size_t write_int32(int32_t sh);
        size_t write_int64(int64_t sh);
        void dump_bytecode();
        void set_address_offset(size_t offset);
        void import(int8_t *bytecode, size_t size);
    private:
        void resize_chunk(size_t min_size);
        const char *get_op_name(enum OpCode op);
        
};
#endif