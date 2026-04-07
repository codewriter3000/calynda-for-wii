#include "runtime_internal.h"

CalyndaRtWord __calynda_rt_union_new(const CalyndaRtTypeDescriptor *type_desc,
                                     uint32_t variant_tag,
                                     CalyndaRtWord payload) {
    CalyndaRtUnion *union_object;

    union_object = calloc(1, sizeof(*union_object));
    if (!union_object) {
        fprintf(stderr, "runtime: out of memory while creating union value\n");
        abort();
    }

    union_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    union_object->header.kind = CALYNDA_RT_OBJECT_UNION;
    union_object->type_desc = type_desc;
    union_object->tag = variant_tag;
    union_object->payload = payload;
    if (!rt_register_object_pointer(union_object)) {
        free(union_object);
        fprintf(stderr, "runtime: out of memory while registering union object\n");
        abort();
    }

    return rt_make_object_word(union_object);
}

uint32_t __calynda_rt_union_get_tag(CalyndaRtWord value) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(value);

    if (!header || header->kind != CALYNDA_RT_OBJECT_UNION) {
        fprintf(stderr, "runtime: attempted tag access on non-union value\n");
        abort();
    }

    return ((const CalyndaRtUnion *)(const void *)header)->tag;
}

CalyndaRtWord __calynda_rt_union_get_payload(CalyndaRtWord value) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(value);

    if (!header || header->kind != CALYNDA_RT_OBJECT_UNION) {
        fprintf(stderr, "runtime: attempted payload access on non-union value\n");
        abort();
    }

    return ((const CalyndaRtUnion *)(const void *)header)->payload;
}

bool __calynda_rt_union_check_tag(CalyndaRtWord value, uint32_t expected_tag) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(value);

    if (!header || header->kind != CALYNDA_RT_OBJECT_UNION) {
        return false;
    }

    return ((const CalyndaRtUnion *)(const void *)header)->tag == expected_tag;
}

CalyndaRtWord __calynda_rt_hetero_array_new(size_t element_count,
                                            const CalyndaRtWord *elements,
                                            const CalyndaRtTypeTag *element_tags) {
    CalyndaRtHeteroArray *array_object;

    array_object = calloc(1, sizeof(*array_object));
    if (!array_object) {
        fprintf(stderr, "runtime: out of memory while creating hetero array\n");
        abort();
    }

    array_object->header.magic = CALYNDA_RT_OBJECT_MAGIC;
    array_object->header.kind = CALYNDA_RT_OBJECT_HETERO_ARRAY;
    array_object->count = element_count;
    if (element_count > 0) {
        array_object->elements = calloc(element_count, sizeof(*array_object->elements));
        array_object->element_tags = calloc(element_count, sizeof(*array_object->element_tags));
        if (!array_object->elements || !array_object->element_tags) {
            free(array_object->elements);
            free(array_object->element_tags);
            free(array_object);
            fprintf(stderr, "runtime: out of memory while creating hetero array elements\n");
            abort();
        }
        if (elements) {
            memcpy(array_object->elements, elements, element_count * sizeof(*elements));
        }
        if (element_tags) {
            memcpy(array_object->element_tags, element_tags, element_count * sizeof(*element_tags));
        }
    }
    if (!rt_register_object_pointer(array_object)) {
        free(array_object->elements);
        free(array_object->element_tags);
        free(array_object);
        fprintf(stderr, "runtime: out of memory while registering hetero array\n");
        abort();
    }

    return rt_make_object_word(array_object);
}

CalyndaRtTypeTag __calynda_rt_hetero_array_get_tag(CalyndaRtWord target, CalyndaRtWord index) {
    const CalyndaRtObjectHeader *header = calynda_rt_as_object(target);
    const CalyndaRtHeteroArray *arr;
    size_t offset;

    if (!header || header->kind != CALYNDA_RT_OBJECT_HETERO_ARRAY) {
        fprintf(stderr, "runtime: attempted hetero-array tag access on non-hetero-array value\n");
        abort();
    }

    arr = (const CalyndaRtHeteroArray *)(const void *)header;
    offset = (size_t)(int64_t)index;
    if (offset >= arr->count) {
        fprintf(stderr, "runtime: hetero-array tag index out of bounds (%zu)\n", offset);
        abort();
    }

    return arr->element_tags[offset];
}
