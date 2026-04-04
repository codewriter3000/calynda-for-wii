#include "bytecode.h"
#include "bytecode_internal.h"

#include <stdlib.h>
#include <string.h>

static char *normalize_literal_text(AstLiteralKind kind, const char *text) {
    char *normalized;
    size_t input_index;
    size_t output_index = 0;
    size_t length;
    char quote;

    if (!text) {
        return NULL;
    }
    if (kind != AST_LITERAL_STRING && kind != AST_LITERAL_CHAR) {
        return ast_copy_text(text);
    }

    length = strlen(text);
    if (length < 2) {
        return ast_copy_text(text);
    }
    quote = kind == AST_LITERAL_CHAR ? '\'' : '"';
    if (text[0] != quote || text[length - 1] != quote) {
        return ast_copy_text(text);
    }

    normalized = malloc(length - 1);
    if (!normalized) {
        return NULL;
    }
    for (input_index = 1; input_index + 1 < length; input_index++) {
        if (text[input_index] == '\\' && input_index + 2 < length) {
            input_index++;
            switch (text[input_index]) {
            case 'n':
                normalized[output_index++] = '\n';
                break;
            case 'r':
                normalized[output_index++] = '\r';
                break;
            case 't':
                normalized[output_index++] = '\t';
                break;
            case '0':
                normalized[output_index++] = '\0';
                break;
            default:
                normalized[output_index++] = text[input_index];
                break;
            }
        } else {
            normalized[output_index++] = text[input_index];
        }
    }
    normalized[output_index] = '\0';
    return normalized;
}

size_t bc_intern_text_constant(BytecodeBuildContext *context,
                               BytecodeConstantKind kind,
                               const char *text) {
    size_t i;
    char *copy;

    if (!context || !context->program || !text) {
        return (size_t)-1;
    }

    for (i = 0; i < context->program->constant_count; i++) {
        if (context->program->constants[i].kind == kind &&
            strcmp(context->program->constants[i].as.text, text) == 0) {
            return i;
        }
    }

    if (!bc_reserve_items((void **)&context->program->constants,
                          &context->program->constant_capacity,
                          context->program->constant_count + 1,
                          sizeof(*context->program->constants))) {
        return (size_t)-1;
    }

    copy = ast_copy_text(text);
    if (!copy) {
        return (size_t)-1;
    }

    i = context->program->constant_count++;
    context->program->constants[i].kind = kind;
    context->program->constants[i].as.text = copy;
    return i;
}

size_t bc_intern_literal_constant(BytecodeBuildContext *context,
                                  AstLiteralKind kind,
                                  const char *text,
                                  bool bool_value) {
    size_t i;
    char *normalized_text = NULL;

    if (!context || !context->program) {
        return (size_t)-1;
    }

    normalized_text = normalize_literal_text(kind, text);
    if (text && !normalized_text) {
        return (size_t)-1;
    }

    for (i = 0; i < context->program->constant_count; i++) {
        const BytecodeConstant *constant = &context->program->constants[i];

        if (constant->kind != BYTECODE_CONSTANT_LITERAL ||
            constant->as.literal.kind != kind ||
            constant->as.literal.bool_value != bool_value) {
            continue;
        }
        if (!constant->as.literal.text && !text) {
            free(normalized_text);
            return i;
        }
        if (constant->as.literal.text && normalized_text &&
            strcmp(constant->as.literal.text, normalized_text) == 0) {
            free(normalized_text);
            return i;
        }
    }

    if (!bc_reserve_items((void **)&context->program->constants,
                          &context->program->constant_capacity,
                          context->program->constant_count + 1,
                          sizeof(*context->program->constants))) {
        return (size_t)-1;
    }

    i = context->program->constant_count++;
    context->program->constants[i].kind = BYTECODE_CONSTANT_LITERAL;
    context->program->constants[i].as.literal.kind = kind;
    context->program->constants[i].as.literal.text = normalized_text;
    context->program->constants[i].as.literal.bool_value = bool_value;
    return i;
}

bool bc_value_from_mir_value(BytecodeBuildContext *context,
                             MirValue value,
                             BytecodeValue *lowered) {
    size_t index;

    if (!lowered) {
        return false;
    }

    *lowered = bc_invalid_value();
    lowered->type = value.type;

    switch (value.kind) {
    case MIR_VALUE_INVALID:
        return true;
    case MIR_VALUE_TEMP:
        lowered->kind = BYTECODE_VALUE_TEMP;
        lowered->as.temp_index = value.as.temp_index;
        return true;
    case MIR_VALUE_LOCAL:
        lowered->kind = BYTECODE_VALUE_LOCAL;
        lowered->as.local_index = value.as.local_index;
        return true;
    case MIR_VALUE_GLOBAL:
        index = bc_intern_text_constant(context, BYTECODE_CONSTANT_SYMBOL, value.as.global_name);
        if (index == (size_t)-1) {
            return false;
        }
        lowered->kind = BYTECODE_VALUE_GLOBAL;
        lowered->as.global_index = index;
        return true;
    case MIR_VALUE_LITERAL:
        index = bc_intern_literal_constant(context,
                                           value.as.literal.kind,
                                           value.as.literal.text,
                                           value.as.literal.bool_value);
        if (index == (size_t)-1) {
            return false;
        }
        lowered->kind = BYTECODE_VALUE_CONSTANT;
        lowered->as.constant_index = index;
        return true;
    }

    return false;
}

bool bc_lower_template_part(BytecodeBuildContext *context,
                            const MirTemplatePart *part,
                            BytecodeTemplatePart *lowered) {
    size_t text_index;

    if (!part || !lowered) {
        return false;
    }

    memset(lowered, 0, sizeof(*lowered));
    if (part->kind == MIR_TEMPLATE_PART_TEXT) {
        text_index = bc_intern_text_constant(context, BYTECODE_CONSTANT_TEXT, part->as.text ? part->as.text : "");
        if (text_index == (size_t)-1) {
            return false;
        }
        lowered->kind = BYTECODE_TEMPLATE_PART_TEXT;
        lowered->as.text_index = text_index;
        return true;
    }

    lowered->kind = BYTECODE_TEMPLATE_PART_VALUE;
    return bc_value_from_mir_value(context, part->as.value, &lowered->as.value);
}
