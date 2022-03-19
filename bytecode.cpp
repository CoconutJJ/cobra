#include "bytecode.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#define WRITE_INT(type, idx, val) *((type *)&this->chunk[idx]) = val
/**
 * Allocate a buffer for the byte code
 */
Bytecode::Bytecode ()
{
        this->capacity = (1 << 8);

        this->chunk = (uint8_t *)malloc (this->capacity * sizeof (uint16_t));

        if (!this->chunk)
                throw -1;

        this->count = 0;
}

/**
 * Resizes the bytecode buffer to at least min_size
 */
void Bytecode::resize_chunk (size_t min_size)
{
        while (this->capacity < min_size)
                this->capacity *= 2;

        this->chunk = (uint8_t *)realloc (this->chunk, this->capacity);

        if (!this->chunk)
                throw -1;
}

/**
 * Write a 8 bit integer
 */
size_t Bytecode::write_uint8 (uint8_t sh)
{
        if (this->count + 1 >= this->capacity)
                this->resize_chunk (this->count + 2);

        WRITE_INT (uint8_t, this->count, sh);
        this->count++;

        return this->count - 1;
}

/**
 * Write a 16 bit integer
 */
size_t Bytecode::write_uint16 (uint16_t sh)
{
        if (this->count + 2 >= this->capacity)
                this->resize_chunk (this->count + 2);

        WRITE_INT (uint16_t, this->count, sh);
        this->count += 2;

        return this->count - 2;
}

/**
 * Write a 32 bit integer
 */
size_t Bytecode::write_uint32 (uint32_t sh)
{
        if (this->count + 4 >= this->capacity)
                this->resize_chunk (this->count + 4);

        WRITE_INT (uint32_t, this->count, sh);
        this->count += 4;

        return this->count - 4;
}

size_t Bytecode::write_uint64 (uint64_t sh)
{
        if (this->count + 8 >= this->capacity)
                this->resize_chunk (this->count + 8);

        WRITE_INT (uint64_t, this->count, sh);
        this->count += 8;

        return this->count - 8;
}

/**
 * Get address of current write position
 */
size_t Bytecode::address ()
{
        return this->count;
}

/**
 * Write an instruction
 */
void Bytecode::emit_op (enum OpCode op)
{
        this->write_uint8 (op & 0xFF);
}

/**
 * Write unconditional jump instruction
 */
size_t Bytecode::emit_jump (uint32_t address)
{
        this->emit_op (OPJMP);
        return this->write_uint32 (address);
}

/**
 * Write conditional false jump instruction
 */
size_t Bytecode::emit_jump_false ()
{
        this->emit_op (OPJMPFALSE);
        return this->write_uint32 (0xFFFFFFFF);
}

/**
 * Patch jump
 */
void Bytecode::patch_jump (size_t offset)
{
        *((uint32_t *)&this->chunk[offset]) = this->count;
}

void Bytecode::dump_bytecode ()
{
#define AS_UINT32(ptr) (*((uint32_t *)ptr))

        size_t c = 0;
        while (c < this->count) {
                uint8_t op_byte = this->chunk[c++];
                enum OpCode op = (enum OpCode)op_byte;

                printf ("%" PRIu64 ": %s ", c - 1, this->get_op_name (op));

                switch (op) {
                case OPJMP:
                case OPJMPFALSE:
                case OPSTORE:
                case OPLOAD:
                case OPPUSH:
                case OPPOP:
                case OPCALL: printf ("%" PRIu32 "\n", AS_UINT32 (&this->chunk[c])); break;
                default: printf ("\n"); continue;
                }
                c += sizeof (uint32_t);
        }
}

const char *Bytecode::get_op_name (enum OpCode op)
{
        switch (op) {
        case OPADD: return "OPADD";
        case OPNEG: return "OPNEG";
        case OPMULT: return "OPMULT";
        case OPDIV: return "OPDIV";
        case OPMOD: return "OPMOD";
        case OPEQ: return "OPEQ";
        case OPGT: return "OPGT";
        case OPLT: return "OPLT";
        case OPGTEQ: return "OPGTEQ";
        case OPLTEQ: return "OPLTEQ";
        case OPAND: return "OPAND";
        case OPOR: return "OPOR";
        case OPJMP: return "OPJMP";
        case OPJMPFALSE: return "OPJMPFALSE";
        case OPSTORE: return "OPSTORE";
        case OPLOAD: return "OPLOAD";
        case OPPUSH: return "OPPUSH";
        case OPPOP: return "OPPOP";
        case OPCALL: return "OPCALL";
        case OPHALT: return "OPHALT";
        case OPBP: return "OPBP";
        default: return "UNKNOWN_OP";
        }
}
