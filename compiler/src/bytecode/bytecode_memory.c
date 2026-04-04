#include "bytecode.h"
#include "bytecode_internal.h"

#include <stdlib.h>
#include <string.h>

bool bc_reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size) {
    void *resized;
    size_t new_capacity;

    if (*capacity >= needed) {
        return true;
    }

    new_capacity = (*capacity == 0) ? 8 : *capacity;
    while (new_capacity < needed) {
        new_capacity *= 2;
    }

    resized = realloc(*items, new_capacity * item_size);
    if (!resized) {
        return false;
    }

    *items = resized;
    *capacity = new_capacity;
    return true;
}

void bc_constant_free(BytecodeConstant *constant) {
    if (!constant) {
        return;
    }

    if (constant->kind == BYTECODE_CONSTANT_LITERAL) {
        free(constant->as.literal.text);
    } else {
        free(constant->as.text);
    }
    memset(constant, 0, sizeof(*constant));
}

void bc_template_part_free(BytecodeTemplatePart *part) {
    if (!part) {
        return;
    }

    memset(part, 0, sizeof(*part));
}

void bc_instruction_free(BytecodeInstruction *instruction) {
    size_t i;

    if (!instruction) {
        return;
    }

    switch (instruction->kind) {
    case BYTECODE_INSTR_CLOSURE:
        free(instruction->as.closure.captures);
        break;
    case BYTECODE_INSTR_CALL:
        free(instruction->as.call.arguments);
        break;
    case BYTECODE_INSTR_ARRAY_LITERAL:
        free(instruction->as.array_literal.elements);
        break;
    case BYTECODE_INSTR_TEMPLATE:
        for (i = 0; i < instruction->as.template_literal.part_count; i++) {
            bc_template_part_free(&instruction->as.template_literal.parts[i]);
        }
        free(instruction->as.template_literal.parts);
        break;
    case BYTECODE_INSTR_BINARY:
    case BYTECODE_INSTR_UNARY:
    case BYTECODE_INSTR_CAST:
    case BYTECODE_INSTR_MEMBER:
    case BYTECODE_INSTR_INDEX_LOAD:
    case BYTECODE_INSTR_STORE_LOCAL:
    case BYTECODE_INSTR_STORE_GLOBAL:
    case BYTECODE_INSTR_STORE_INDEX:
    case BYTECODE_INSTR_STORE_MEMBER:
    case BYTECODE_INSTR_UNION_NEW:
    case BYTECODE_INSTR_UNION_GET_TAG:
    case BYTECODE_INSTR_UNION_GET_PAYLOAD:
        break;
    case BYTECODE_INSTR_HETERO_ARRAY_NEW:
        free(instruction->as.hetero_array_new.elements);
        free(instruction->as.hetero_array_new.element_tags);
        break;
    case BYTECODE_INSTR_HETERO_ARRAY_GET_TAG:
        break;
    }

    memset(instruction, 0, sizeof(*instruction));
}

void bc_block_free(BytecodeBasicBlock *block) {
    size_t i;

    if (!block) {
        return;
    }

    free(block->label);
    for (i = 0; i < block->instruction_count; i++) {
        bc_instruction_free(&block->instructions[i]);
    }
    free(block->instructions);
    memset(block, 0, sizeof(*block));
}

void bc_unit_free(BytecodeUnit *unit) {
    size_t i;

    if (!unit) {
        return;
    }

    free(unit->name);
    for (i = 0; i < unit->local_count; i++) {
        free(unit->locals[i].name);
    }
    free(unit->locals);

    for (i = 0; i < unit->block_count; i++) {
        bc_block_free(&unit->blocks[i]);
    }
    free(unit->blocks);
    memset(unit, 0, sizeof(*unit));
}
