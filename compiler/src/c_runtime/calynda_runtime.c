/*
 * calynda_runtime.c — combined 32-bit runtime implementation.
 *
 * Combines the logic of runtime.c, runtime_names.c, runtime_format.c,
 * runtime_ops.c and runtime_union.c but targets a 32-bit word size
 * (CalyndaRtWord = uint32_t) and uses the GC allocator for heap objects.
 */

#include "calynda_runtime.h"
#include "calynda_wpad.h"
#include "calynda_gc.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Object registry (for calynda_rt_as_object validation)              */
/* ------------------------------------------------------------------ */

typedef struct {
    void  **items;
    size_t  count;
    size_t  capacity;
} RuntimeObjectRegistry;

static RuntimeObjectRegistry OBJECT_REGISTRY;

CalyndaRtExternCallable STDOUT_PRINT_CALLABLE = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_EXTERN_CALLABLE },
    CALYNDA_RT_EXTERN_CALL_STDOUT_PRINT,
    "print"
};

CalyndaRtPackage __calynda_pkg_stdlib = {
    { CALYNDA_RT_OBJECT_MAGIC, CALYNDA_RT_OBJECT_PACKAGE },
    "stdlib"
};

static bool registry_reserve(size_t needed) {
    void  **resized;
    size_t  new_cap;

    if (OBJECT_REGISTRY.capacity >= needed) {
        return true;
    }
    new_cap = (OBJECT_REGISTRY.capacity == 0) ? 8 : OBJECT_REGISTRY.capacity;
    while (new_cap < needed) {
        new_cap *= 2;
    }
    resized = realloc(OBJECT_REGISTRY.items, new_cap * sizeof(void *));
    if (!resized) {
        return false;
    }
    OBJECT_REGISTRY.items = resized;
    OBJECT_REGISTRY.capacity = new_cap;
    return true;
}

static bool registry_contains(const void *ptr) {
    size_t i;

    for (i = 0; i < OBJECT_REGISTRY.count; i++) {
        if (OBJECT_REGISTRY.items[i] == ptr) {
            return true;
        }
    }
    return false;
}

static bool rt_register_object_pointer(void *ptr) {
    if (!ptr || registry_contains(ptr)) {
        return true;
    }
    if (!registry_reserve(OBJECT_REGISTRY.count + 1)) {
        return false;
    }
    OBJECT_REGISTRY.items[OBJECT_REGISTRY.count++] = ptr;
    return true;
}

static CalyndaRtWord rt_make_object_word(void *ptr) {
    return (CalyndaRtWord)(uintptr_t)ptr;
}

/* ------------------------------------------------------------------ */
/* Internal buffer helpers                                              */
/* ------------------------------------------------------------------ */

static bool buf_reserve(char **buf, size_t *len, size_t *cap, size_t extra) {
    char  *resized;
    size_t new_cap;

    if (*cap >= *len + extra + 1) {
        return true;
    }
    new_cap = (*cap == 0) ? 64 : *cap;
    while (new_cap < *len + extra + 1) {
        new_cap *= 2;
    }
    resized = realloc(*buf, new_cap);
    if (!resized) {
        return false;
    }
    *buf = resized;
    *cap = new_cap;
    return true;
}

static bool buf_append_text(char **buf, size_t *len, size_t *cap, const char *text) {
    size_t tlen;

    text = text ? text : "";
    tlen = strlen(text);
    if (!buf_reserve(buf, len, cap, tlen)) {
        return false;
    }
    memcpy(*buf + *len, text, tlen);
    *len += tlen;
    (*buf)[*len] = '\0';
    return true;
}

/* ------------------------------------------------------------------ */
/* String helpers                                                       */
/* ------------------------------------------------------------------ */

static char *copy_bytes_n(const char *src, size_t len) {
    char *dst = malloc(len + 1);

    if (!dst) {
        return NULL;
    }
    if (len > 0) {
        memcpy(dst, src, len);
    }
    dst[len] = '\0';
    return dst;
}

/* ------------------------------------------------------------------ */
/* Object constructors                                                  */
/* ------------------------------------------------------------------ */

static CalyndaRtString *rt_new_string_object(const char *bytes, size_t length) {
    CalyndaRtString *obj;
    char            *copied;

    copied = copy_bytes_n(bytes, length);
    if (!copied) {
        return NULL;
    }

    obj = calynda_gc_alloc(sizeof(*obj));
    if (!obj) {
        free(copied);
        return NULL;
    }

    obj->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    obj->header.kind  = CALYNDA_RT_OBJECT_STRING;
    obj->length       = length;
    obj->bytes        = copied;

    if (!rt_register_object_pointer(obj)) {
        free(copied);
        calynda_gc_release(obj);
        return NULL;
    }
    return obj;
}

static CalyndaRtArray *rt_new_array_object(size_t count, const CalyndaRtWord *elements) {
    CalyndaRtArray *obj;

    obj = calynda_gc_alloc(sizeof(*obj));
    if (!obj) {
        return NULL;
    }

    obj->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    obj->header.kind  = CALYNDA_RT_OBJECT_ARRAY;
    obj->count        = count;

    if (count > 0) {
        obj->elements = calloc(count, sizeof(*obj->elements));
        if (!obj->elements) {
            calynda_gc_release(obj);
            return NULL;
        }
        if (elements) {
            memcpy(obj->elements, elements, count * sizeof(*elements));
        }
    }

    if (!rt_register_object_pointer(obj)) {
        free(obj->elements);
        calynda_gc_release(obj);
        return NULL;
    }
    return obj;
}

static CalyndaRtClosure *rt_new_closure_object(CalyndaRtClosureEntry entry,
                                                size_t capture_count,
                                                const CalyndaRtWord *captures) {
    CalyndaRtClosure *obj;

    obj = calynda_gc_alloc(sizeof(*obj));
    if (!obj) {
        return NULL;
    }

    obj->header.magic  = CALYNDA_RT_OBJECT_MAGIC;
    obj->header.kind   = CALYNDA_RT_OBJECT_CLOSURE;
    obj->entry         = entry;
    obj->capture_count = capture_count;

    if (capture_count > 0) {
        obj->captures = calloc(capture_count, sizeof(*obj->captures));
        if (!obj->captures) {
            calynda_gc_release(obj);
            return NULL;
        }
        if (captures) {
            memcpy(obj->captures, captures, capture_count * sizeof(*captures));
        }
    }

    if (!rt_register_object_pointer(obj)) {
        free(obj->captures);
        calynda_gc_release(obj);
        return NULL;
    }
    return obj;
}

/* ------------------------------------------------------------------ */
/* Public: object inspection                                            */
/* ------------------------------------------------------------------ */

bool calynda_rt_is_object(CalyndaRtWord word) {
    return calynda_rt_as_object(word) != NULL;
}

const CalyndaRtObjectHeader *calynda_rt_as_object(CalyndaRtWord word) {
    const void *ptr = (const void *)(uintptr_t)word;

    if (word == 0) {
        return NULL;
    }
    if (ptr == (const void *)&__calynda_pkg_stdlib ||
        ptr == (const void *)&STDOUT_PRINT_CALLABLE ||
        ptr == (const void *)&__calynda_pkg_wpad) {
        return (const CalyndaRtObjectHeader *)ptr;
    }
    if (!registry_contains(ptr)) {
        return NULL;
    }
    return (const CalyndaRtObjectHeader *)ptr;
}

CalyndaRtWord calynda_rt_make_string_copy(const char *bytes) {
    CalyndaRtString *obj;

    if (!bytes) {
        bytes = "";
    }
    obj = rt_new_string_object(bytes, strlen(bytes));
    if (!obj) {
        fprintf(stderr, "runtime: out of memory while creating string\n");
        abort();
    }
    return rt_make_object_word(obj);
}

void calynda_rt_register_static_object(const CalyndaRtObjectHeader *object) {
    if (!object || object->magic != CALYNDA_RT_OBJECT_MAGIC) {
        return;
    }
    rt_register_object_pointer((void *)(uintptr_t)object);
}

/* ------------------------------------------------------------------ */
/* Names                                                                */
/* ------------------------------------------------------------------ */

const char *calynda_rt_object_kind_name(CalyndaRtObjectKind kind) {
    switch (kind) {
    case CALYNDA_RT_OBJECT_STRING:          return "string";
    case CALYNDA_RT_OBJECT_ARRAY:           return "array";
    case CALYNDA_RT_OBJECT_CLOSURE:         return "closure";
    case CALYNDA_RT_OBJECT_PACKAGE:         return "package";
    case CALYNDA_RT_OBJECT_EXTERN_CALLABLE: return "extern-callable";
    case CALYNDA_RT_OBJECT_UNION:           return "union";
    case CALYNDA_RT_OBJECT_HETERO_ARRAY:    return "hetero-array";
    }
    return "unknown";
}

const char *calynda_rt_type_tag_name(CalyndaRtTypeTag tag) {
    switch (tag) {
    case CALYNDA_RT_TYPE_VOID:         return "void";
    case CALYNDA_RT_TYPE_BOOL:         return "bool";
    case CALYNDA_RT_TYPE_INT32:        return "int32";
    case CALYNDA_RT_TYPE_INT64:        return "int64";
    case CALYNDA_RT_TYPE_STRING:       return "string";
    case CALYNDA_RT_TYPE_ARRAY:        return "array";
    case CALYNDA_RT_TYPE_CLOSURE:      return "closure";
    case CALYNDA_RT_TYPE_EXTERNAL:     return "external";
    case CALYNDA_RT_TYPE_RAW_WORD:     return "raw-word";
    case CALYNDA_RT_TYPE_UNION:        return "union";
    case CALYNDA_RT_TYPE_HETERO_ARRAY: return "hetero-array";
    }
    return "unknown";
}

bool calynda_rt_dump_layout(FILE *out) {
    if (!out) {
        return false;
    }
    fprintf(out,
            "RuntimeLayout word=uint32 raw-scalar-or-object-handle\n"
            "  ObjectHeader size=%zu magic=0x%08X fields=[magic:uint32, kind:uint32]\n"
            "  String size=%zu payload=[length:size_t, bytes:char*]\n"
            "  Array size=%zu payload=[count:size_t, elements:uint32*]\n"
            "  Closure size=%zu payload=[entry:void*, capture_count:size_t, captures:uint32*]\n"
            "  Package size=%zu payload=[name:char*]\n"
            "  ExternCallable size=%zu payload=[kind:uint32, name:char*]\n"
            "  TemplatePart size=%zu payload=[tag:uint32, payload:uint32]\n"
            "  TemplateTags text=%d value=%d\n"
            "  Union size=%zu payload=[type_desc:TypeDescriptor*, tag:uint32, payload:uint32]\n"
            "  HeteroArray size=%zu payload=[count:size_t, elements:uint32*, element_tags:uint32*]\n"
            "  TypeDescriptor fields=[name:char*, generic_param_count:size_t, variant_count:size_t, variant_names:char**, variant_payload_tags:uint32*]\n"
            "  Builtins package=stdlib member=print\n",
            sizeof(CalyndaRtWord),
            CALYNDA_RT_OBJECT_MAGIC,
            sizeof(CalyndaRtString),
            sizeof(CalyndaRtArray),
            sizeof(CalyndaRtClosure),
            sizeof(CalyndaRtPackage),
            sizeof(CalyndaRtExternCallable),
            sizeof(CalyndaRtTemplatePart),
            CALYNDA_RT_TEMPLATE_TAG_TEXT,
            CALYNDA_RT_TEMPLATE_TAG_VALUE,
            sizeof(CalyndaRtUnion),
            sizeof(CalyndaRtHeteroArray));
    return !ferror(out);
}

char *calynda_rt_dump_layout_to_string(void) {
    FILE  *stream;
    char  *buffer;
    long   size;
    size_t read_size;

    stream = tmpfile();
    if (!stream) {
        return NULL;
    }
    if (!calynda_rt_dump_layout(stream) || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_END) != 0) {
        fclose(stream);
        return NULL;
    }
    size = ftell(stream);
    if (size < 0 || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }
    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(stream);
        return NULL;
    }
    read_size = fread(buffer, 1, (size_t)size, stream);
    fclose(stream);
    if (read_size != (size_t)size) {
        free(buffer);
        return NULL;
    }
    buffer[size] = '\0';
    return buffer;
}

/* ------------------------------------------------------------------ */
/* Format                                                               */
/* ------------------------------------------------------------------ */

static bool rt_format_word_internal(CalyndaRtWord word, char *buf, size_t bufsz);

static long long rt_signed_from_word(CalyndaRtWord value) {
    return (long long)(int32_t)value;
}

static CalyndaRtWord rt_word_from_signed(long long value) {
    return (CalyndaRtWord)(int32_t)value;
}

static bool rt_is_runtime_string_word(CalyndaRtWord word) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(word);

    return h && h->kind == CALYNDA_RT_OBJECT_STRING;
}

static bool rt_format_word_internal(CalyndaRtWord word, char *buf, size_t bufsz) {
    const CalyndaRtObjectHeader *h;

    if (!buf || bufsz == 0) {
        return false;
    }

    h = calynda_rt_as_object(word);
    if (!h) {
        return snprintf(buf, bufsz, "%lld", rt_signed_from_word(word)) >= 0;
    }

    switch ((CalyndaRtObjectKind)h->kind) {
    case CALYNDA_RT_OBJECT_STRING:
        return snprintf(buf, bufsz, "%s",
                        ((const CalyndaRtString *)(const void *)h)->bytes) >= 0;
    case CALYNDA_RT_OBJECT_ARRAY:
        return snprintf(buf, bufsz, "<array:%zu>",
                        ((const CalyndaRtArray *)(const void *)h)->count) >= 0;
    case CALYNDA_RT_OBJECT_CLOSURE:
        return snprintf(buf, bufsz, "<closure>") >= 0;
    case CALYNDA_RT_OBJECT_PACKAGE:
        return snprintf(buf, bufsz, "<package:%s>",
                        ((const CalyndaRtPackage *)(const void *)h)->name) >= 0;
    case CALYNDA_RT_OBJECT_EXTERN_CALLABLE:
        return snprintf(buf, bufsz, "<extern:%s>",
                        ((const CalyndaRtExternCallable *)(const void *)h)->name) >= 0;
    case CALYNDA_RT_OBJECT_UNION: {
        const CalyndaRtUnion *u = (const CalyndaRtUnion *)(const void *)h;
        const char *vname = "?";

        if (u->type_desc && u->tag < u->type_desc->variant_count) {
            vname = u->type_desc->variant_names[u->tag];
        }
        return snprintf(buf, bufsz, "<%s::%s>",
                        u->type_desc ? u->type_desc->name : "union",
                        vname) >= 0;
    }
    case CALYNDA_RT_OBJECT_HETERO_ARRAY:
        return snprintf(buf, bufsz, "<hetero-array:%zu>",
                        ((const CalyndaRtHeteroArray *)(const void *)h)->count) >= 0;
    }
    return false;
}

bool calynda_rt_format_word(CalyndaRtWord word, char *buffer, size_t buffer_size) {
    return rt_format_word_internal(word, buffer, buffer_size);
}

const char *calynda_rt_string_bytes(CalyndaRtWord word) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(word);

    if (!h || h->kind != CALYNDA_RT_OBJECT_STRING) {
        return NULL;
    }
    return ((const CalyndaRtString *)(const void *)h)->bytes;
}

size_t calynda_rt_array_length(CalyndaRtWord word) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(word);

    if (!h || h->kind != CALYNDA_RT_OBJECT_ARRAY) {
        return 0;
    }
    return ((const CalyndaRtArray *)(const void *)h)->count;
}

/* ------------------------------------------------------------------ */
/* Extern callable dispatch                                             */
/* ------------------------------------------------------------------ */

static CalyndaRtWord rt_dispatch_extern_callable(const CalyndaRtExternCallable *callable,
                                                  size_t argument_count,
                                                  const CalyndaRtWord *arguments) {
    char buf[256];

    if (!callable) {
        fprintf(stderr, "runtime: missing extern callable dispatch target\n");
        abort();
    }

    switch (callable->kind) {
    case CALYNDA_RT_EXTERN_CALL_STDOUT_PRINT:
        if (argument_count > 0) {
            if (!rt_format_word_internal(arguments[0], buf, sizeof(buf))) {
                strcpy(buf, "<unformattable>");
            }
            printf("%s\n", buf);
        } else {
            printf("\n");
        }
        return 0;
    }

    fprintf(stderr, "runtime: unsupported extern callable kind %d\n",
            (int)callable->kind);
    abort();
}

/* ------------------------------------------------------------------ */
/* Template builder                                                     */
/* ------------------------------------------------------------------ */

CalyndaRtWord __calynda_rt_template_build(size_t part_count,
                                          const CalyndaRtTemplatePart *parts) {
    char   *buffer   = NULL;
    size_t  length   = 0;
    size_t  capacity = 0;
    size_t  i;

    for (i = 0; i < part_count; i++) {
        if (parts[i].tag == CALYNDA_RT_TEMPLATE_TAG_TEXT) {
            const char *text = (const char *)(uintptr_t)parts[i].payload;

            if (!buf_append_text(&buffer, &length, &capacity, text)) {
                free(buffer);
                fprintf(stderr, "runtime: out of memory in template build\n");
                abort();
            }
        } else if (parts[i].tag == CALYNDA_RT_TEMPLATE_TAG_VALUE) {
            char word_text[256];

            if (!rt_format_word_internal(parts[i].payload, word_text, sizeof(word_text))) {
                free(buffer);
                fprintf(stderr, "runtime: failed to format template value\n");
                abort();
            }
            if (!buf_append_text(&buffer, &length, &capacity, word_text)) {
                free(buffer);
                fprintf(stderr, "runtime: out of memory in template build\n");
                abort();
            }
        } else {
            free(buffer);
            fprintf(stderr, "runtime: unknown template tag (%u)\n",
                    (unsigned)parts[i].tag);
            abort();
        }
    }

    if (!buffer) {
        return calynda_rt_make_string_copy("");
    }
    {
        CalyndaRtString *obj = rt_new_string_object(buffer, length);

        free(buffer);
        if (!obj) {
            fprintf(stderr, "runtime: out of memory boxing template result\n");
            abort();
        }
        return rt_make_object_word(obj);
    }
}

/* ------------------------------------------------------------------ */
/* Ops                                                                  */
/* ------------------------------------------------------------------ */

CalyndaRtWord __calynda_rt_closure_new(CalyndaRtClosureEntry code_ptr,
                                       size_t capture_count,
                                       const CalyndaRtWord *captures) {
    CalyndaRtClosure *obj = rt_new_closure_object(code_ptr, capture_count, captures);

    if (!obj) {
        fprintf(stderr, "runtime: out of memory creating closure\n");
        abort();
    }
    return rt_make_object_word(obj);
}

CalyndaRtWord __calynda_rt_call_callable(CalyndaRtWord callable,
                                         size_t argument_count,
                                         const CalyndaRtWord *arguments) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(callable);

    if (!h) {
        fprintf(stderr, "runtime: attempted to call a non-callable raw word (%u)\n",
                (unsigned)callable);
        abort();
    }

    switch ((CalyndaRtObjectKind)h->kind) {
    case CALYNDA_RT_OBJECT_CLOSURE: {
        const CalyndaRtClosure *cl = (const CalyndaRtClosure *)(const void *)h;

        return cl->entry(cl->captures, cl->capture_count, arguments, argument_count);
    }
    case CALYNDA_RT_OBJECT_EXTERN_CALLABLE: {
        const CalyndaRtExternCallable *ec =
            (const CalyndaRtExternCallable *)(const void *)h;
        if ((int)ec->kind >= 100) {
            return calynda_wpad_dispatch(ec, argument_count, arguments);
        }
        return rt_dispatch_extern_callable(ec, argument_count, arguments);
    }
    default:
        fprintf(stderr, "runtime: object of kind %s is not callable\n",
                calynda_rt_object_kind_name((CalyndaRtObjectKind)h->kind));
        abort();
    }
}

CalyndaRtWord __calynda_rt_member_load(CalyndaRtWord target, const char *member) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(target);

    if (!h) {
        fprintf(stderr, "runtime: member access on non-object value\n");
        abort();
    }
    if (!member) {
        fprintf(stderr, "runtime: member access with null member symbol\n");
        abort();
    }
    if (h == &__calynda_pkg_stdlib.header && strcmp(member, "print") == 0) {
        return rt_make_object_word(&STDOUT_PRINT_CALLABLE);
    }
    if (h == &__calynda_pkg_wpad.header) {
        CalyndaRtWord result = calynda_wpad_member_load(member);
        if (result != 0) return result;
    }
    fprintf(stderr, "runtime: unsupported member load .%s\n", member);
    abort();
}

CalyndaRtWord __calynda_rt_index_load(CalyndaRtWord target, CalyndaRtWord index) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(target);
    size_t offset;

    if (!h || h->kind != CALYNDA_RT_OBJECT_ARRAY) {
        fprintf(stderr, "runtime: index load on non-array value\n");
        abort();
    }
    offset = (size_t)rt_signed_from_word(index);
    if (offset >= ((const CalyndaRtArray *)(const void *)h)->count) {
        fprintf(stderr, "runtime: array index out of bounds (%zu)\n", offset);
        abort();
    }
    return ((const CalyndaRtArray *)(const void *)h)->elements[offset];
}

CalyndaRtWord __calynda_rt_array_literal(size_t count, const CalyndaRtWord *elements) {
    CalyndaRtArray *obj = rt_new_array_object(count, elements);

    if (!obj) {
        fprintf(stderr, "runtime: out of memory creating array\n");
        abort();
    }
    return rt_make_object_word(obj);
}

void __calynda_rt_store_index(CalyndaRtWord target, CalyndaRtWord index, CalyndaRtWord value) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(target);
    size_t offset;

    if (!h || h->kind != CALYNDA_RT_OBJECT_ARRAY) {
        fprintf(stderr, "runtime: index store on non-array value\n");
        abort();
    }
    offset = (size_t)rt_signed_from_word(index);
    if (offset >= ((const CalyndaRtArray *)(const void *)h)->count) {
        fprintf(stderr, "runtime: array index out of bounds (%zu)\n", offset);
        abort();
    }
    ((CalyndaRtArray *)(void *)h)->elements[offset] = value;
}

void __calynda_rt_store_member(CalyndaRtWord target, const char *member, CalyndaRtWord value) {
    (void)target; (void)member; (void)value;
    fprintf(stderr, "runtime: member stores are not implemented\n");
    abort();
}

void __calynda_rt_throw(CalyndaRtWord value) {
    char buf[256];

    if (!rt_format_word_internal(value, buf, sizeof(buf))) {
        strcpy(buf, "<unformattable>");
    }
    fprintf(stderr, "Unhandled throw: %s\n", buf);
    abort();
}

CalyndaRtWord __calynda_rt_cast_value(CalyndaRtWord source, CalyndaRtTypeTag target_type_tag) {
    switch (target_type_tag) {
    case CALYNDA_RT_TYPE_BOOL:
        return source != 0 ? 1 : 0;

    case CALYNDA_RT_TYPE_INT32:
    case CALYNDA_RT_TYPE_INT64: {
        const CalyndaRtObjectHeader *h = calynda_rt_as_object(source);

        if (h && h->kind == CALYNDA_RT_OBJECT_STRING) {
            return rt_word_from_signed(
                strtoll(((const CalyndaRtString *)(const void *)h)->bytes, NULL, 10));
        }
        return source;
    }

    case CALYNDA_RT_TYPE_STRING: {
        char buf[128];

        if (rt_is_runtime_string_word(source)) {
            return source;
        }
        if (!rt_format_word_internal(source, buf, sizeof(buf))) {
            fprintf(stderr, "runtime: failed to stringify cast source\n");
            abort();
        }
        return calynda_rt_make_string_copy(buf);
    }

    default:
        return source;
    }
}

void calynda_rt_init(void) {
    /* Register WPAD static objects so they pass as_object validation. */
    calynda_wpad_register_objects();
}

int calynda_rt_start_process(CalyndaRtProgramStartEntry entry, int argc, char **argv) {
    CalyndaRtWord *elements = NULL;
    CalyndaRtWord  arguments;
    size_t         argument_count = 0;
    size_t         i;
    CalyndaRtWord  result;

    if (!entry) {
        fprintf(stderr, "runtime: missing native start entry\n");
        return 70;
    }

    calynda_rt_init();

    if (argc > 1) {
        argument_count = (size_t)(argc - 1);
        elements = calloc(argument_count, sizeof(*elements));
        if (!elements) {
            fprintf(stderr, "runtime: out of memory boxing process arguments\n");
            return 71;
        }
        for (i = 0; i < argument_count; i++) {
            elements[i] = calynda_rt_make_string_copy(argv[i + 1]);
        }
    }

    arguments = __calynda_rt_array_literal(argument_count, elements);
    free(elements);
    result = entry(arguments);
    return (int)(int32_t)result;
}

/* ------------------------------------------------------------------ */
/* Union                                                                */
/* ------------------------------------------------------------------ */

CalyndaRtWord __calynda_rt_union_new(const CalyndaRtTypeDescriptor *type_desc,
                                     uint32_t variant_tag,
                                     CalyndaRtWord payload) {
    CalyndaRtUnion *obj = calynda_gc_alloc(sizeof(*obj));

    if (!obj) {
        fprintf(stderr, "runtime: out of memory creating union\n");
        abort();
    }
    obj->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    obj->header.kind  = CALYNDA_RT_OBJECT_UNION;
    obj->type_desc    = type_desc;
    obj->tag          = variant_tag;
    obj->payload      = payload;
    if (!rt_register_object_pointer(obj)) {
        calynda_gc_release(obj);
        fprintf(stderr, "runtime: out of memory registering union\n");
        abort();
    }
    return rt_make_object_word(obj);
}

uint32_t __calynda_rt_union_get_tag(CalyndaRtWord value) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(value);

    if (!h || h->kind != CALYNDA_RT_OBJECT_UNION) {
        fprintf(stderr, "runtime: tag access on non-union value\n");
        abort();
    }
    return ((const CalyndaRtUnion *)(const void *)h)->tag;
}

CalyndaRtWord __calynda_rt_union_get_payload(CalyndaRtWord value) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(value);

    if (!h || h->kind != CALYNDA_RT_OBJECT_UNION) {
        fprintf(stderr, "runtime: payload access on non-union value\n");
        abort();
    }
    return ((const CalyndaRtUnion *)(const void *)h)->payload;
}

bool __calynda_rt_union_check_tag(CalyndaRtWord value, uint32_t expected_tag) {
    const CalyndaRtObjectHeader *h = calynda_rt_as_object(value);

    if (!h || h->kind != CALYNDA_RT_OBJECT_UNION) {
        return false;
    }
    return ((const CalyndaRtUnion *)(const void *)h)->tag == expected_tag;
}

CalyndaRtWord __calynda_rt_hetero_array_new(size_t count,
                                            const CalyndaRtWord *elements,
                                            const CalyndaRtTypeTag *element_tags) {
    CalyndaRtHeteroArray *obj = calynda_gc_alloc(sizeof(*obj));

    if (!obj) {
        fprintf(stderr, "runtime: out of memory creating hetero-array\n");
        abort();
    }
    obj->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    obj->header.kind  = CALYNDA_RT_OBJECT_HETERO_ARRAY;
    obj->count        = count;

    if (count > 0) {
        obj->elements     = calloc(count, sizeof(*obj->elements));
        obj->element_tags = calloc(count, sizeof(*obj->element_tags));
        if (!obj->elements || !obj->element_tags) {
            free(obj->elements);
            free(obj->element_tags);
            calynda_gc_release(obj);
            fprintf(stderr, "runtime: out of memory creating hetero-array elements\n");
            abort();
        }
        if (elements) {
            memcpy(obj->elements, elements, count * sizeof(*elements));
        }
        if (element_tags) {
            memcpy(obj->element_tags, element_tags, count * sizeof(*element_tags));
        }
    }
    if (!rt_register_object_pointer(obj)) {
        free(obj->elements);
        free(obj->element_tags);
        calynda_gc_release(obj);
        fprintf(stderr, "runtime: out of memory registering hetero-array\n");
        abort();
    }
    return rt_make_object_word(obj);
}

CalyndaRtTypeTag __calynda_rt_hetero_array_get_tag(CalyndaRtWord target, CalyndaRtWord index) {
    const CalyndaRtObjectHeader  *h   = calynda_rt_as_object(target);
    const CalyndaRtHeteroArray   *arr;
    size_t                        off;

    if (!h || h->kind != CALYNDA_RT_OBJECT_HETERO_ARRAY) {
        fprintf(stderr, "runtime: hetero-array tag access on non-hetero-array\n");
        abort();
    }
    arr = (const CalyndaRtHeteroArray *)(const void *)h;
    off = (size_t)(int32_t)index;
    if (off >= arr->count) {
        fprintf(stderr, "runtime: hetero-array tag index out of bounds (%zu)\n", off);
        abort();
    }
    return arr->element_tags[off];
}
