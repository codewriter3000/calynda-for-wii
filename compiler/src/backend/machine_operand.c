#include "machine_internal.h"

#include <stdlib.h>
#include <string.h>

bool mc_format_capture_operand(size_t capture_index, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("env(%zu)", capture_index);
    return *text != NULL;
}

bool mc_format_helper_slot_operand(size_t helper_slot_index, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("helper(%zu)", helper_slot_index);
    return *text != NULL;
}

bool mc_format_incoming_arg_stack_operand(size_t argument_index, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("argin(%zu)", argument_index);
    return *text != NULL;
}

bool mc_format_outgoing_arg_stack_operand(size_t argument_index, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("argout(%zu)", argument_index);
    return *text != NULL;
}

bool mc_format_member_symbol_operand(const char *member, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("symbol(%s)", member ? member : "");
    return *text != NULL;
}

bool mc_format_code_label_operand(const char *unit_name, char **text) {
    if (!text) {
        return false;
    }

    *text = mc_copy_format("code(%s)", unit_name ? unit_name : "");
    return *text != NULL;
}

bool mc_format_template_text_operand(const char *text, char **formatted) {
    if (!formatted) {
        return false;
    }

    *formatted = mc_copy_format("text(\"%s\")", text ? text : "");
    return *formatted != NULL;
}

bool mc_emit_move_to_destination(MachineBuildContext *context,
                                 MachineBlock *block,
                                 const char *destination,
                                 const char *source) {
    return mc_append_line(context, block, "mov %s, %s", destination, source);
}

bool mc_emit_store_vreg(MachineBuildContext *context,
                        const CodegenUnit *codegen_unit,
                        size_t vreg_index,
                        MachineBlock *block,
                        const char *source) {
    const CodegenVRegAllocation *allocation;

    if (!codegen_unit || vreg_index >= codegen_unit->vreg_count) {
        mc_set_error(context,
                     (AstSourceSpan){0},
                     NULL,
                     "Machine emission missing virtual-register allocation %zu.",
                     vreg_index);
        return false;
    }

    allocation = &codegen_unit->vreg_allocations[vreg_index];
    if (allocation->location.kind == CODEGEN_VREG_REGISTER) {
        if (strcmp(codegen_register_name(allocation->location.as.reg), source) == 0) {
            return true;
        }
        return mc_append_line(context,
                              block,
                              "mov %s, %s",
                              codegen_register_name(allocation->location.as.reg),
                              source);
    }

    if (strcmp(codegen_register_name(CODEGEN_REG_R14), source) == 0) {
        return mc_append_line(context,
                              block,
                              "mov spill(%zu), %s",
                              allocation->location.as.spill_slot_index,
                              codegen_register_name(CODEGEN_REG_R14));
    }

    return mc_append_line(context, block, "mov %s, %s", codegen_register_name(CODEGEN_REG_R14), source) &&
           mc_append_line(context,
                          block,
                          "mov spill(%zu), %s",
                          allocation->location.as.spill_slot_index,
                          codegen_register_name(CODEGEN_REG_R14));
}

bool mc_emit_preserve_before_call(MachineBuildContext *context,
                                  const CodegenUnit *codegen_unit,
                                  MachineBlock *block) {
    if (mc_unit_uses_register(codegen_unit, CODEGEN_REG_R10) &&
        !mc_append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R10))) {
        return false;
    }
    if (mc_unit_uses_register(codegen_unit, CODEGEN_REG_R11) &&
        !mc_append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R11))) {
        return false;
    }
    return true;
}

bool mc_emit_preserve_after_call(MachineBuildContext *context,
                                 const CodegenUnit *codegen_unit,
                                 MachineBlock *block) {
    if (mc_unit_uses_register(codegen_unit, CODEGEN_REG_R11) &&
        !mc_append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R11))) {
        return false;
    }
    if (mc_unit_uses_register(codegen_unit, CODEGEN_REG_R10) &&
        !mc_append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R10))) {
        return false;
    }
    return true;
}

bool mc_emit_entry_prologue(MachineBuildContext *context,
                            const CodegenUnit *codegen_unit,
                            MachineUnit *machine_unit,
                            MachineBlock *block) {
    size_t stack_words;

    stack_words = mc_unit_stack_word_count(machine_unit);
    if (!mc_append_line(context, block, "push rbp") ||
        !mc_append_line(context, block, "mov rbp, rsp") ||
        !mc_append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R14))) {
        return false;
    }
    if (mc_unit_uses_register(codegen_unit, CODEGEN_REG_R12) &&
        !mc_append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R12))) {
        return false;
    }
    if (mc_unit_uses_register(codegen_unit, CODEGEN_REG_R13) &&
        !mc_append_line(context, block, "push %s", codegen_register_name(CODEGEN_REG_R13))) {
        return false;
    }
    if (stack_words > 0 &&
        !mc_append_line(context, block, "sub rsp, %zu", stack_words * 8)) {
        return false;
    }
    return true;
}

bool mc_emit_return_epilogue(MachineBuildContext *context,
                             const CodegenUnit *codegen_unit,
                             const MachineUnit *machine_unit,
                             MachineBlock *block) {
    size_t stack_words;

    stack_words = mc_unit_stack_word_count(machine_unit);
    if (stack_words > 0 &&
        !mc_append_line(context, block, "add rsp, %zu", stack_words * 8)) {
        return false;
    }
    if (mc_unit_uses_register(codegen_unit, CODEGEN_REG_R13) &&
        !mc_append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R13))) {
        return false;
    }
    if (mc_unit_uses_register(codegen_unit, CODEGEN_REG_R12) &&
        !mc_append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R12))) {
        return false;
    }
    if (!mc_append_line(context, block, "pop %s", codegen_register_name(CODEGEN_REG_R14)) ||
        !mc_append_line(context, block, "pop rbp") ||
        !mc_append_line(context, block, "ret")) {
        return false;
    }
    return true;
}
