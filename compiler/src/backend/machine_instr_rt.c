#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

bool mc_emit_instruction_runtime(MachineBuildContext *context,
                                 const LirUnit *lir_unit,
                                 const CodegenUnit *codegen_unit,
                                 const LirInstruction *instruction,
                                 const CodegenSelectedInstruction *selected,
                                 MachineBlock *block) {
    switch (selected->selection.as.runtime_helper) {
    case CODEGEN_RUNTIME_CLOSURE_NEW: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        size_t i;
        bool ok = true;

        for (i = 0; i < instruction->as.closure.capture_count; i++) {
            char *slot = NULL;
            char *value = NULL;

            ok = mc_format_helper_slot_operand(i, &slot) &&
                 mc_format_operand(lir_unit, codegen_unit, instruction->as.closure.captures[i], &value) &&
                 mc_emit_move_to_destination(context, block, slot, value);
            free(slot);
            free(value);
            if (!ok) {
                return false;
            }
        }

        {
            char *unit_label = NULL;
            bool pointer_ok;

            if (!mc_format_code_label_operand(instruction->as.closure.unit_name, &unit_label)) {
                return false;
            }
            pointer_ok = instruction->as.closure.capture_count > 0
                ? mc_append_line(context, block, "lea rdx, helper(0)")
                : mc_append_line(context, block, "mov rdx, null");
            ok = mc_append_line(context, block, "mov rdi, %s", unit_label) &&
                 mc_append_line(context,
                                block,
                                "mov rsi, %zu",
                                instruction->as.closure.capture_count) &&
                 pointer_ok &&
                 mc_emit_runtime_helper_call(context,
                                             codegen_unit,
                                             signature,
                                             block,
                                             false) &&
                 mc_emit_store_vreg(context,
                                    codegen_unit,
                                    instruction->as.closure.dest_vreg,
                                    block,
                                    "rax");
            free(unit_label);
            return ok;
        }
    }
    case CODEGEN_RUNTIME_CALL_CALLABLE: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *callee = NULL;
        bool ok;

        if (!mc_format_operand(lir_unit, codegen_unit, instruction->as.call.callee, &callee)) {
            return false;
        }
        ok = mc_append_line(context, block, "mov rdi, %s", callee) &&
             mc_append_line(context, block, "mov rsi, %zu", instruction->as.call.argument_count) &&
             (instruction->as.call.argument_count > 0
                  ? mc_append_line(context, block, "lea rdx, helper(0)")
                  : mc_append_line(context, block, "mov rdx, null")) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false);
        free(callee);
        if (!ok) {
            return false;
        }
        if (instruction->as.call.has_result) {
            return mc_emit_store_vreg(context,
                                      codegen_unit,
                                      instruction->as.call.dest_vreg,
                                      block,
                                      "rax");
        }
        return true;
    }
    case CODEGEN_RUNTIME_MEMBER_LOAD: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *member = NULL;
        bool ok;

        if (!mc_format_operand(lir_unit, codegen_unit, instruction->as.member.target, &target) ||
            !mc_format_member_symbol_operand(instruction->as.member.member, &member)) {
            free(target);
            free(member);
            return false;
        }
        ok = mc_append_line(context, block, "mov rdi, %s", target) &&
             mc_append_line(context, block, "mov rsi, %s", member) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.member.dest_vreg,
                                block,
                                "rax");
        free(target);
        free(member);
        return ok;
    }
    case CODEGEN_RUNTIME_INDEX_LOAD: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        char *target = NULL;
        char *index = NULL;
        bool ok;

        if (!mc_format_operand(lir_unit, codegen_unit, instruction->as.index_load.target, &target) ||
            !mc_format_operand(lir_unit, codegen_unit, instruction->as.index_load.index, &index)) {
            free(target);
            free(index);
            return false;
        }
        ok = mc_append_line(context, block, "mov rdi, %s", target) &&
             mc_append_line(context, block, "mov rsi, %s", index) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.index_load.dest_vreg,
                                block,
                                "rax");
        free(target);
        free(index);
        return ok;
    }
    case CODEGEN_RUNTIME_ARRAY_LITERAL: {
        const RuntimeAbiHelperSignature *signature =
            runtime_abi_get_helper_signature(context->program->target,
                                             selected->selection.as.runtime_helper);
        size_t i;
        bool ok = true;

        for (i = 0; i < instruction->as.array_literal.element_count; i++) {
            char *slot = NULL;
            char *value = NULL;

            ok = mc_format_helper_slot_operand(i, &slot) &&
                 mc_format_operand(lir_unit, codegen_unit, instruction->as.array_literal.elements[i], &value) &&
                 mc_emit_move_to_destination(context, block, slot, value);
            free(slot);
            free(value);
            if (!ok) {
                return false;
            }
        }
        ok = mc_append_line(context,
                            block,
                            "mov rdi, %zu",
                            instruction->as.array_literal.element_count) &&
             (instruction->as.array_literal.element_count > 0
                  ? mc_append_line(context, block, "lea rsi, helper(0)")
                  : mc_append_line(context, block, "mov rsi, null")) &&
             mc_emit_runtime_helper_call(context, codegen_unit, signature, block, false) &&
             mc_emit_store_vreg(context,
                                codegen_unit,
                                instruction->as.array_literal.dest_vreg,
                                block,
                                "rax");
        return ok;
    }
    default:
        return mc_emit_instruction_runtime_ext(context, lir_unit, codegen_unit,
                                               instruction, selected, block);
    }
}
