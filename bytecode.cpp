#include "bytecode.h"
#include <stdio.h>
#include <stdlib.h>
#define WRITE_INT(type, idx, val) *((type *)&this->chunk[idx]) = val
Bytecode::Bytecode ()
{
        this->capacity = (1 << 8);

        this->chunk = (uint8_t *)malloc (this->capacity * sizeof (uint16_t));

        if (!this->chunk)
                throw -1;

        this->count = 0;
}

void Bytecode::resize_chunk (size_t min_size)
{
        while (this->capacity < min_size)
                this->capacity *= 2;

        this->chunk = (uint8_t *)realloc (this->chunk, this->capacity);

        if (!this->chunk)
                throw -1;
}

size_t Bytecode::write_uint8 (uint8_t sh)
{
        if (this->count + 1 >= this->capacity)
                this->resize_chunk (this->count + 2);

        WRITE_INT (uint8_t, this->count, sh);
        this->count++;

        return this->count - 1;
}

size_t Bytecode::write_uint16 (uint16_t sh)
{
        if (this->count + 2 >= this->capacity)
                this->resize_chunk (this->count + 2);

        WRITE_INT (uint16_t, this->count, sh);
        this->count += 2;

        return this->count - 2;
}

size_t Bytecode::write_uint32 (uint32_t sh)
{
        if (this->count + 4 >= this->capacity)
                this->resize_chunk (this->count + 4);

        WRITE_INT (uint32_t, this->count, sh);
        this->count += 4;

        return this->count - 4;
}

size_t Bytecode::address ()
{
        return this->count;
}

void Bytecode::emit_op (enum OpCode op)
{
        this->write_uint8 (op & 0xFF);
}

size_t Bytecode::emit_jump (uint32_t address)
{
        this->emit_op (OPJMP);
        return this->write_uint32 (address);
}

size_t Bytecode::emit_jump_false ()
{
        this->emit_op (OPJMPFALSE);
        return this->write_uint32 (0xFFFFFFFF);
}

void Bytecode::patch_jump (size_t offset)
{
        *((uint32_t *)&this->chunk[offset]) = this->count;
}