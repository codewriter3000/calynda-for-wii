#include "codegen_internal.h"

static void assign_vreg_type(CheckedType *types,
                             size_t type_count,
                             size_t vreg_index,
                             CheckedType type) {
    if (!types || vreg_index >= type_count || type.kind == CHECKED_TYPE_INVALID) {
        return;
    }

    if (types[vreg_index].kind == CHECKED_TYPE_INVALID) {
        types[vreg_index] = type;
    }
}

static void propagate_operand_vreg_type(CheckedType *types,
                                        size_t type_count,
                                        LirOperand operand) {
    if (operand.kind == LIR_OPERAND_VREG) {
        assign_vreg_type(types, type_count, operand.as.vreg_index, operand.type);
    }
}

void cg_infer_vreg_types(const LirUnit *lir_unit,
                         CheckedType *types,
                         size_t type_count) {
    size_t block_index;

    if (!lir_unit || !types) {
        return;
    }

    for (block_index = 0; block_index < lir_unit->block_count; block_index++) {
        const LirBasicBlock *block = &lir_unit->blocks[block_index];
        size_t instruction_index;

        for (instruction_index = 0;
             instruction_index < block->instruction_count;
             instruction_index++) {
            const LirInstruction *instruction = &block->instructions[instruction_index];

            switch (instruction->kind) {
            case LIR_INSTR_INCOMING_ARG:
            case LIR_INSTR_INCOMING_CAPTURE:
                break;

            case LIR_INSTR_OUTGOING_ARG:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.outgoing_arg.value);
                break;

            case LIR_INSTR_BINARY:
                assign_vreg_type(types,
                                 type_count,
                                 instruction->as.binary.dest_vreg,
                                 instruction->as.binary.left.type);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.binary.left);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.binary.right);
                break;

            case LIR_INSTR_UNARY:
                assign_vreg_type(types,
                                 type_count,
                                 instruction->as.unary.dest_vreg,
                                 instruction->as.unary.operand.type);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.unary.operand);
                break;

            case LIR_INSTR_CLOSURE:
                for (size_t capture_index = 0;
                     capture_index < instruction->as.closure.capture_count;
                     capture_index++) {
                    propagate_operand_vreg_type(types,
                                                type_count,
                                                instruction->as.closure.captures[capture_index]);
                }
                break;

            case LIR_INSTR_CALL:
                if (instruction->as.call.has_result) {
                    assign_vreg_type(types,
                                     type_count,
                                     instruction->as.call.dest_vreg,
                                     instruction->as.call.callee.type);
                }
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.call.callee);
                break;

            case LIR_INSTR_CAST:
                assign_vreg_type(types,
                                 type_count,
                                 instruction->as.cast.dest_vreg,
                                 instruction->as.cast.target_type);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.cast.operand);
                break;

            case LIR_INSTR_MEMBER:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.member.target);
                break;

            case LIR_INSTR_INDEX_LOAD:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.index_load.target);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.index_load.index);
                break;

            case LIR_INSTR_ARRAY_LITERAL:
                for (size_t element_index = 0;
                     element_index < instruction->as.array_literal.element_count;
                     element_index++) {
                    propagate_operand_vreg_type(types,
                                                type_count,
                                                instruction->as.array_literal.elements[element_index]);
                }
                break;

            case LIR_INSTR_TEMPLATE:
                for (size_t part_index = 0;
                     part_index < instruction->as.template_literal.part_count;
                     part_index++) {
                    if (instruction->as.template_literal.parts[part_index].kind ==
                        LIR_TEMPLATE_PART_VALUE) {
                        propagate_operand_vreg_type(types,
                                                    type_count,
                                                    instruction->as.template_literal.parts[part_index].as.value);
                    }
                }
                break;

            case LIR_INSTR_STORE_SLOT:
                if (instruction->as.store_slot.value.kind == LIR_OPERAND_VREG &&
                    instruction->as.store_slot.slot_index < lir_unit->slot_count) {
                    assign_vreg_type(types,
                                     type_count,
                                     instruction->as.store_slot.value.as.vreg_index,
                                     lir_unit->slots[instruction->as.store_slot.slot_index].type);
                }
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_slot.value);
                break;

            case LIR_INSTR_STORE_GLOBAL:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_global.value);
                break;

            case LIR_INSTR_STORE_INDEX:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_index.target);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_index.index);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_index.value);
                break;

            case LIR_INSTR_STORE_MEMBER:
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_member.target);
                propagate_operand_vreg_type(types,
                                            type_count,
                                            instruction->as.store_member.value);
                break;

            case LIR_INSTR_HETERO_ARRAY_NEW:
                for (size_t element_index = 0;
                     element_index < instruction->as.hetero_array_new.element_count;
                     element_index++) {
                    propagate_operand_vreg_type(types,
                                                type_count,
                                                instruction->as.hetero_array_new.elements[element_index]);
                }
                break;

            case LIR_INSTR_UNION_NEW:
                if (instruction->as.union_new.has_payload) {
                    propagate_operand_vreg_type(types,
                                                type_count,
                                                instruction->as.union_new.payload);
                }
                break;
            }
        }

        switch (block->terminator.kind) {
        case LIR_TERM_RETURN:
            if (block->terminator.as.return_term.has_value) {
                propagate_operand_vreg_type(types,
                                            type_count,
                                            block->terminator.as.return_term.value);
            }
            break;

        case LIR_TERM_BRANCH:
            propagate_operand_vreg_type(types,
                                        type_count,
                                        block->terminator.as.branch_term.condition);
            break;

        case LIR_TERM_THROW:
            propagate_operand_vreg_type(types,
                                        type_count,
                                        block->terminator.as.throw_term.value);
            break;

        case LIR_TERM_NONE:
        case LIR_TERM_JUMP:
            break;
        }
    }
}
