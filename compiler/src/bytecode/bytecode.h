#ifndef CALYNDA_BYTECODE_H
#define CALYNDA_BYTECODE_H

#include "bytecode_instr_types.h"

typedef enum {
    BYTECODE_TERM_NONE = 0,
    BYTECODE_TERM_RETURN,
    BYTECODE_TERM_JUMP,
    BYTECODE_TERM_BRANCH,
    BYTECODE_TERM_THROW
} BytecodeTerminatorKind;

typedef struct {
    BytecodeTerminatorKind kind;
    union {
        struct {
            bool          has_value;
            BytecodeValue value;
        } return_term;
        struct {
            size_t target_block;
        } jump_term;
        struct {
            BytecodeValue condition;
            size_t        true_block;
            size_t        false_block;
        } branch_term;
        struct {
            BytecodeValue value;
        } throw_term;
    } as;
} BytecodeTerminator;

struct BytecodeBasicBlock {
    char                *label;
    BytecodeInstruction *instructions;
    size_t               instruction_count;
    BytecodeTerminator   terminator;
};

typedef enum {
    BYTECODE_UNIT_START = 0,
    BYTECODE_UNIT_BINDING,
    BYTECODE_UNIT_INIT,
    BYTECODE_UNIT_LAMBDA
} BytecodeUnitKind;

struct BytecodeUnit {
    BytecodeUnitKind   kind;
    char              *name;
    const Symbol      *symbol;
    CheckedType        return_type;
    BytecodeLocal     *locals;
    size_t             local_count;
    size_t             parameter_count;
    size_t             temp_count;
    BytecodeBasicBlock *blocks;
    size_t             block_count;
};

typedef struct {
    AstSourceSpan primary_span;
    AstSourceSpan related_span;
    bool          has_related_span;
    char          message[256];
} BytecodeBuildError;

typedef struct {
    BytecodeTargetKind target;
    BytecodeConstant  *constants;
    size_t             constant_count;
    size_t             constant_capacity;
    BytecodeUnit      *units;
    size_t             unit_count;
    BytecodeBuildError error;
    bool               has_error;
} BytecodeProgram;

void bytecode_program_init(BytecodeProgram *program);
void bytecode_program_free(BytecodeProgram *program);

bool bytecode_build_program(BytecodeProgram *program, const MirProgram *mir_program);

const BytecodeBuildError *bytecode_get_error(const BytecodeProgram *program);
bool bytecode_format_error(const BytecodeBuildError *error,
                           char *buffer,
                           size_t buffer_size);
const char *bytecode_target_name(BytecodeTargetKind target);

bool bytecode_dump_program(FILE *out, const BytecodeProgram *program);
char *bytecode_dump_program_to_string(const BytecodeProgram *program);

#endif /* CALYNDA_BYTECODE_H */