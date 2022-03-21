#include "bytecode.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
/**
 * Allocate a buffer for the byte code
 */
Bytecode::Bytecode ()
{
        this->capacity = (1 << 8);

        this->chunk = (int8_t *)malloc (this->capacity * sizeof (int16_t));

        if (!this->chunk)
                throw -1;

        this->count = 0;
        this->address_offset = 0;
}

/**
 * Resizes the bytecode buffer to at least min_size
 */
void Bytecode::resize_chunk (size_t min_size)
{
        while (this->capacity < min_size)
                this->capacity *= 2;

        this->chunk = (int8_t *)realloc (this->chunk, this->capacity);

        if (!this->chunk)
                throw -1;
}

/**
 * Write a 8 bit integer
 */
size_t Bytecode::write_int8 (int8_t sh)
{
        if (this->count + sizeof (int8_t) >= this->capacity)
                this->resize_chunk (this->count + sizeof (int8_t));

        WRITE_INT (int8_t, this->count, sh);
        this->count += sizeof (int8_t);

        return this->count - sizeof (int8_t);
}

/**
 * Write a 16 bit integer
 */
size_t Bytecode::write_int16 (int16_t sh)
{
        if (this->count + sizeof (int16_t) >= this->capacity)
                this->resize_chunk (this->count + sizeof (int16_t));

        WRITE_INT (int16_t, this->count, sh);
        this->count += sizeof (int16_t);

        return this->count - sizeof (int16_t);
}

/**
 * Write a 32 bit integer
 */
size_t Bytecode::write_int32 (int32_t sh)
{
        if (this->count + sizeof (int32_t) >= this->capacity)
                this->resize_chunk (this->count + sizeof (int32_t));

        WRITE_INT (int32_t, this->count, sh);
        this->count += sizeof (int32_t);

        return this->count - sizeof (int32_t);
}

size_t Bytecode::write_int64 (int64_t sh)
{
        if (this->count + sizeof (int64_t) >= this->capacity)
                this->resize_chunk (this->count + sizeof (int64_t));

        WRITE_INT (int64_t, this->count, sh);
        this->count += sizeof (int64_t);

        return this->count - sizeof (int64_t);
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
        this->write_int8 (op & 0xFF);
}

/**
 * Write unconditional jump instruction
 */
size_t Bytecode::emit_jump (int32_t address)
{
        this->emit_op (OPJMP);
        return this->write_int32 (address);
}

/**
 * Write conditional false jump instruction
 */
size_t Bytecode::emit_jump_false ()
{
        this->emit_op (OPJMPFALSE);
        return this->write_int32 (0xFFFFFFFF);
}

/**
 * Patch jump
 */
void Bytecode::patch_jump (size_t offset)
{
        *((int32_t *)&this->chunk[offset]) = this->count;
}

bool Bytecode::instruction_at (size_t *position, enum OpCode *opcode, int32_t *arg)
{
        if (*position >= this->count)
                return false;

        int8_t op_byte = this->chunk[(*position)++];
        enum OpCode op = (enum OpCode)op_byte;

        *opcode = op;

        switch (op) {
        case OPJMP:
        case OPJMPFALSE:
        case OPSTORE:
        case OPLOAD:
        case OPCALL:
        case OPPUSH: {
                *arg = AS_INT32 (&this->chunk[*position]);
                *position += sizeof (int32_t);
        } break;
        default: break;
        }
        return true;
}

void Bytecode::dump_bytecode ()
{
        size_t c = 0;
        enum OpCode op;
        int32_t arg;
        while (this->instruction_at (&c, &op, &arg)) {
                printf ("%" PRIu64 ": %s ", c - 1, this->get_op_name (op));

                switch (op) {
                case OPJMP:
                case OPJMPFALSE:
                case OPSTORE:
                case OPLOAD:
                case OPCALL:
                case OPPUSH: {
                        printf ("%d\n", arg);
                        break;
                }
                default: printf ("\n"); continue;
                }
        }
}

void Bytecode::set_address_offset (size_t offset)
{
        size_t c = 0;
        while (c < this->count) {
                int8_t op_byte = this->chunk[c++];
                enum OpCode op = (enum OpCode)op_byte;

                switch (op) {
                case OPJMPFALSE:
                case OPJMP: {
                        int32_t old_offset = AS_INT32 (&this->chunk[c]);
                        WRITE_INT (int32_t, c, old_offset - this->address_offset + offset);
                        c += sizeof (int32_t);
                        break;
                }
                default: continue;
                }
        }
}

void Bytecode::import (int8_t *bytecode, size_t size)
{
        this->resize_chunk (this->count + size);

        while (size > 0) {
                this->chunk[this->count++] = *bytecode;
                bytecode++;
                size--;
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
        case OPSTOREBP: return "OPSTOREBP";
        case OPPUSHBP: return "OPPUSHBP";
        case OPSTORESP: return "OPSTORESP";
        case OPPUSHSP: return "OPPUSHSP";
        case OPRET: return "OPRET";
        default: return "UNKNOWN_OP";
        }
}
