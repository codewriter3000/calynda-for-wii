#ifndef LIR_INTERNAL_H
#define LIR_INTERNAL_H

#include "lir.h"

typedef struct {
    LirProgram      *program;
    const MirProgram *mir_program;
} LirBuildContext;

/* lir_memory.c */
bool lr_reserve_items(void **items, size_t *capacity, size_t needed, size_t item_size);
void lr_operand_free(LirOperand *operand);
void lr_template_part_free(LirTemplatePart *part);
void lr_instruction_free(LirInstruction *instruction);
void lr_basic_block_free(LirBasicBlock *block);
void lr_unit_free(LirUnit *unit);

/* lir_helpers.c */
bool lr_source_span_is_valid(AstSourceSpan span);
void lr_set_error(LirBuildContext *context,
                  AstSourceSpan primary_span,
                  const AstSourceSpan *related_span,
                  const char *format,
                  ...);
LirOperand lr_invalid_operand(void);
bool lr_operand_from_mir_value(LirBuildContext *context,
                               const LirUnit *unit,
                               MirValue value,
                               LirOperand *operand);
LirSlotKind lr_slot_kind_from_mir(MirLocalKind kind);
LirUnitKind lr_unit_kind_from_mir(MirUnitKind kind);
bool lr_append_unit(LirProgram *program, LirUnit unit);
bool lr_append_slot(LirUnit *unit, LirSlot slot);
bool lr_append_block(LirUnit *unit, LirBasicBlock block);
bool lr_append_instruction(LirBasicBlock *block, LirInstruction instruction);

/* lir_lower_instr.c */
bool lr_lower_mir_instruction(LirBuildContext *context,
                              const MirUnit *mir_unit,
                              const LirUnit *unit,
                              LirBasicBlock *block,
                              const MirInstruction *instruction);

/* lir_lower_instr_ext.c */
bool lr_lower_mir_instr_ext(LirBuildContext *context,
                            const MirUnit *mir_unit,
                            const LirUnit *unit,
                            LirBasicBlock *block,
                            const MirInstruction *instruction);

/* lir_lower_instr_stores.c */
bool lr_lower_mir_instr_stores(LirBuildContext *context,
                               const MirUnit *mir_unit,
                               const LirUnit *unit,
                               LirBasicBlock *block,
                               const MirInstruction *instruction);

/* lir_lower.c */
bool lr_emit_entry_abi(LirBuildContext *context,
                       const LirUnit *unit,
                       LirBasicBlock *block);
bool lr_lower_mir_unit(LirBuildContext *context,
                       const MirUnit *mir_unit,
                       LirUnit *unit);

#endif
