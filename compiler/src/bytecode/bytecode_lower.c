#include "bytecode.h"
#include "bytecode_internal.h"

#include <stdlib.h>
#include <string.h>

static bool lower_terminator(BytecodeBuildContext *context,
                             const MirTerminator *terminator,
                             BytecodeTerminator *lowered) {
    if (!terminator || !lowered) {
        return false;
    }

    memset(lowered, 0, sizeof(*lowered));
    lowered->kind = (BytecodeTerminatorKind)terminator->kind;

    switch (terminator->kind) {
    case MIR_TERM_NONE:
        return true;
    case MIR_TERM_RETURN:
        lowered->as.return_term.has_value = terminator->as.return_term.has_value;
        if (!lowered->as.return_term.has_value) {
            return true;
        }
        return bc_value_from_mir_value(context,
                                       terminator->as.return_term.value,
                                       &lowered->as.return_term.value);
    case MIR_TERM_GOTO:
        lowered->as.jump_term.target_block = terminator->as.goto_term.target_block;
        return true;
    case MIR_TERM_BRANCH:
        lowered->as.branch_term.true_block = terminator->as.branch_term.true_block;
        lowered->as.branch_term.false_block = terminator->as.branch_term.false_block;
        return bc_value_from_mir_value(context,
                                       terminator->as.branch_term.condition,
                                       &lowered->as.branch_term.condition);
    case MIR_TERM_THROW:
        return bc_value_from_mir_value(context,
                                       terminator->as.throw_term.value,
                                       &lowered->as.throw_term.value);
    }

    return false;
}

bool bc_lower_unit(BytecodeBuildContext *context,
                   const MirUnit *mir_unit,
                   BytecodeUnit *unit) {
    size_t local_index;
    size_t block_index;

    if (!context || !mir_unit || !unit) {
        return false;
    }

    memset(unit, 0, sizeof(*unit));
    unit->kind = bc_unit_kind_from_mir(mir_unit->kind);
    unit->name = ast_copy_text(mir_unit->name);
    unit->symbol = mir_unit->symbol;
    unit->return_type = mir_unit->return_type;
    unit->local_count = mir_unit->local_count;
    unit->parameter_count = mir_unit->parameter_count;
    unit->temp_count = mir_unit->next_temp_index;
    unit->block_count = mir_unit->block_count;
    if (!unit->name) {
        bc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Out of memory while naming bytecode unit.");
        return false;
    }

    if (unit->local_count > 0) {
        unit->locals = calloc(unit->local_count, sizeof(*unit->locals));
        if (!unit->locals) {
            bc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while allocating bytecode locals.");
            return false;
        }
    }
    for (local_index = 0; local_index < unit->local_count; local_index++) {
        unit->locals[local_index].kind = bc_local_kind_from_mir(mir_unit->locals[local_index].kind);
        unit->locals[local_index].name = ast_copy_text(mir_unit->locals[local_index].name);
        unit->locals[local_index].symbol = mir_unit->locals[local_index].symbol;
        unit->locals[local_index].type = mir_unit->locals[local_index].type;
        unit->locals[local_index].is_final = mir_unit->locals[local_index].is_final;
        unit->locals[local_index].index = mir_unit->locals[local_index].index;
        if (!unit->locals[local_index].name) {
            bc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while naming bytecode local.");
            return false;
        }
    }

    if (unit->block_count > 0) {
        unit->blocks = calloc(unit->block_count, sizeof(*unit->blocks));
        if (!unit->blocks) {
            bc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while allocating bytecode blocks.");
            return false;
        }
    }
    for (block_index = 0; block_index < unit->block_count; block_index++) {
        const MirBasicBlock *mir_block = &mir_unit->blocks[block_index];
        BytecodeBasicBlock *block = &unit->blocks[block_index];
        size_t instruction_index;

        block->label = ast_copy_text(mir_block->label);
        block->instruction_count = mir_block->instruction_count;
        if (!block->label) {
            bc_set_error(context,
                         (AstSourceSpan){0},
                         NULL,
                         "Out of memory while naming bytecode block.");
            return false;
        }
        if (block->instruction_count > 0) {
            block->instructions = calloc(block->instruction_count, sizeof(*block->instructions));
            if (!block->instructions) {
                bc_set_error(context,
                             (AstSourceSpan){0},
                             NULL,
                             "Out of memory while allocating bytecode instructions.");
                return false;
            }
        }
        for (instruction_index = 0; instruction_index < block->instruction_count; instruction_index++) {
            if (!bc_lower_instruction(context,
                                      &mir_block->instructions[instruction_index],
                                      &block->instructions[instruction_index])) {
                return false;
            }
        }
        if (!lower_terminator(context, &mir_block->terminator, &block->terminator)) {
            return false;
        }
    }

    return true;
}
