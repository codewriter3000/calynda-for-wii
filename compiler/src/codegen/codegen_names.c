#include "codegen_internal.h"

const char *codegen_target_name(CodegenTargetKind target) {
    switch (target) {
    case CODEGEN_TARGET_X86_64_SYSV_ELF:
        return "x86_64_sysv_elf";
    }

    return "unknown";
}

const char *codegen_register_name(CodegenRegister reg) {
    switch (reg) {
    case CODEGEN_REG_RAX:
        return "rax";
    case CODEGEN_REG_RDI:
        return "rdi";
    case CODEGEN_REG_RSI:
        return "rsi";
    case CODEGEN_REG_RDX:
        return "rdx";
    case CODEGEN_REG_RCX:
        return "rcx";
    case CODEGEN_REG_R8:
        return "r8";
    case CODEGEN_REG_R9:
        return "r9";
    case CODEGEN_REG_R10:
        return "r10";
    case CODEGEN_REG_R11:
        return "r11";
    case CODEGEN_REG_R12:
        return "r12";
    case CODEGEN_REG_R13:
        return "r13";
    case CODEGEN_REG_R14:
        return "r14";
    case CODEGEN_REG_R15:
        return "r15";
    }

    return "?";
}

const char *codegen_direct_pattern_name(CodegenDirectPattern pattern) {
    switch (pattern) {
    case CODEGEN_DIRECT_ABI_ARG_MOVE:
        return "abi-arg-move";
    case CODEGEN_DIRECT_ABI_CAPTURE_LOAD:
        return "abi-capture-load";
    case CODEGEN_DIRECT_ABI_OUTGOING_ARG:
        return "abi-outgoing-arg";
    case CODEGEN_DIRECT_SCALAR_BINARY:
        return "scalar-binary";
    case CODEGEN_DIRECT_SCALAR_UNARY:
        return "scalar-unary";
    case CODEGEN_DIRECT_SCALAR_CAST:
        return "scalar-cast";
    case CODEGEN_DIRECT_CALL_GLOBAL:
        return "call-global";
    case CODEGEN_DIRECT_STORE_SLOT:
        return "store-slot";
    case CODEGEN_DIRECT_STORE_GLOBAL:
        return "store-global";
    case CODEGEN_DIRECT_RETURN:
        return "return";
    case CODEGEN_DIRECT_JUMP:
        return "jump";
    case CODEGEN_DIRECT_BRANCH:
        return "branch";
    }

    return "unknown-direct";
}

const char *codegen_runtime_helper_name(CodegenRuntimeHelper helper) {
    switch (helper) {
    case CODEGEN_RUNTIME_CLOSURE_NEW:
        return "__calynda_rt_closure_new";
    case CODEGEN_RUNTIME_CALL_CALLABLE:
        return "__calynda_rt_call_callable";
    case CODEGEN_RUNTIME_MEMBER_LOAD:
        return "__calynda_rt_member_load";
    case CODEGEN_RUNTIME_INDEX_LOAD:
        return "__calynda_rt_index_load";
    case CODEGEN_RUNTIME_ARRAY_LITERAL:
        return "__calynda_rt_array_literal";
    case CODEGEN_RUNTIME_TEMPLATE_BUILD:
        return "__calynda_rt_template_build";
    case CODEGEN_RUNTIME_STORE_INDEX:
        return "__calynda_rt_store_index";
    case CODEGEN_RUNTIME_STORE_MEMBER:
        return "__calynda_rt_store_member";
    case CODEGEN_RUNTIME_THROW:
        return "__calynda_rt_throw";
    case CODEGEN_RUNTIME_CAST_VALUE:
        return "__calynda_rt_cast_value";
    case CODEGEN_RUNTIME_UNION_NEW:
        return "__calynda_rt_union_new";
    case CODEGEN_RUNTIME_UNION_GET_TAG:
        return "__calynda_rt_union_get_tag";
    case CODEGEN_RUNTIME_UNION_GET_PAYLOAD:
        return "__calynda_rt_union_get_payload";
    case CODEGEN_RUNTIME_HETERO_ARRAY_NEW:
        return "__calynda_rt_hetero_array_new";
    case CODEGEN_RUNTIME_HETERO_ARRAY_GET_TAG:
        return "__calynda_rt_hetero_array_get_tag";
    }

    return "__calynda_rt_unknown";
}
