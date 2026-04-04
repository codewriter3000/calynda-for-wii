#include "c_emit_internal.h"
#include "ast_types.h"
#include "type_checker.h"

#include <stdio.h>
#include <string.h>

/* Map a CheckedType to a C type string written to 'out'. */
void c_emit_type(FILE *out, CheckedType type) {
    if (type.array_depth > 0) {
        /* Arrays are represented as CalyndaRtWord (handle) */
        fputs("CalyndaRtWord", out);
        return;
    }

    switch (type.kind) {
    case CHECKED_TYPE_VOID:
        fputs("void", out);
        return;
    case CHECKED_TYPE_NULL:
        fputs("void *", out);
        return;
    case CHECKED_TYPE_NAMED:
    case CHECKED_TYPE_EXTERNAL:
    case CHECKED_TYPE_TYPE_PARAM:
        /* Named/union/external types are opaque words */
        fputs("CalyndaRtWord", out);
        return;
    case CHECKED_TYPE_VALUE:
        break;
    case CHECKED_TYPE_INVALID:
        fputs("CalyndaRtWord", out);
        return;
    }

    /* CHECKED_TYPE_VALUE — map primitive */
    switch (type.primitive) {
    case AST_PRIMITIVE_INT8:
    case AST_PRIMITIVE_SBYTE:
        fputs("int8_t", out);   return;
    case AST_PRIMITIVE_INT16:
    case AST_PRIMITIVE_SHORT:
        fputs("int16_t", out);  return;
    case AST_PRIMITIVE_INT32:
    case AST_PRIMITIVE_INT:
        fputs("int32_t", out);  return;
    case AST_PRIMITIVE_INT64:
    case AST_PRIMITIVE_LONG:
        fputs("int64_t", out);  return;
    case AST_PRIMITIVE_UINT8:
    case AST_PRIMITIVE_BYTE:
        fputs("uint8_t", out);  return;
    case AST_PRIMITIVE_UINT16:
        fputs("uint16_t", out); return;
    case AST_PRIMITIVE_UINT32:
    case AST_PRIMITIVE_UINT:
        fputs("uint32_t", out); return;
    case AST_PRIMITIVE_UINT64:
    case AST_PRIMITIVE_ULONG:
        fputs("uint64_t", out); return;
    case AST_PRIMITIVE_FLOAT32:
    case AST_PRIMITIVE_FLOAT:
        fputs("float", out);    return;
    case AST_PRIMITIVE_FLOAT64:
    case AST_PRIMITIVE_DOUBLE:
        fputs("double", out);   return;
    case AST_PRIMITIVE_BOOL:
        fputs("bool", out);     return;
    case AST_PRIMITIVE_CHAR:
        fputs("char", out);     return;
    case AST_PRIMITIVE_STRING:
        /* strings are GC-managed objects, represented as words */
        fputs("CalyndaRtWord", out); return;
    }

    /* Fallback */
    fputs("CalyndaRtWord", out);
}

void c_emit_type_str(char *buf, size_t bufsz, CheckedType type) {
    FILE *f;
    long  sz;
    size_t rsz;

    f = tmpfile();
    if (!f) {
        if (bufsz > 0) {
            snprintf(buf, bufsz, "CalyndaRtWord");
        }
        return;
    }
    c_emit_type(f, type);
    fflush(f);
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0 || (size_t)sz + 1 > bufsz) {
        fclose(f);
        if (bufsz > 0) {
            snprintf(buf, bufsz, "CalyndaRtWord");
        }
        return;
    }
    rsz = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rsz] = '\0';
}
