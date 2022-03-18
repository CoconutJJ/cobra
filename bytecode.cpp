#include "bytecode.h"
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

size_t Bytecode::write_uint64 (uint64_t sh){
        if (this->count + 8 >= this->capacity)
                this->resize_chunk(this->count + 8);
        
        WRITE_INT(uint64_t, this->count, sh);
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