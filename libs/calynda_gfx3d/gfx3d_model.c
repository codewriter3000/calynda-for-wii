/*
 * gfx3d_model.c — .c3d binary model loading.
 */

#include "gfx3d_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Gfx3dModel *gfx3d_model_load_c3d_mem(const void *data, uint32_t size) {
    if (size < sizeof(C3dHeader)) return NULL;

    const C3dHeader *hdr = (const C3dHeader *)data;
    if (hdr->magic != C3D_MAGIC || hdr->version != C3D_VERSION) return NULL;

    const uint8_t *base = (const uint8_t *)data;

    /* Validate offsets are within buffer */
    uint32_t vert_end = hdr->vertex_data_offset + hdr->vertex_count * sizeof(Gfx3dVertex);
    uint32_t idx_end = hdr->index_data_offset + hdr->index_count * sizeof(uint16_t);
    if (vert_end > size || idx_end > size) return NULL;

    const Gfx3dVertex *verts = (const Gfx3dVertex *)(base + hdr->vertex_data_offset);
    const uint16_t *indices = (const uint16_t *)(base + hdr->index_data_offset);

    Gfx3dModel *model = calloc(1, sizeof(Gfx3dModel));
    if (!model) return NULL;

    model->mesh = gfx3d_mesh_create(verts, hdr->vertex_count, indices, hdr->index_count);
    if (!model->mesh) { free(model); return NULL; }

    /* Override computed bounds with stored bounds */
    model->mesh->bounds.min.x = hdr->bounds_min[0];
    model->mesh->bounds.min.y = hdr->bounds_min[1];
    model->mesh->bounds.min.z = hdr->bounds_min[2];
    model->mesh->bounds.max.x = hdr->bounds_max[0];
    model->mesh->bounds.max.y = hdr->bounds_max[1];
    model->mesh->bounds.max.z = hdr->bounds_max[2];

    /* Read material name if available */
    if (hdr->material_name_offset > 0 && hdr->material_name_offset < size) {
        const char *name = (const char *)(base + hdr->material_name_offset);
        /* Safely copy, ensuring null termination */
        size_t max_len = size - hdr->material_name_offset;
        if (max_len > sizeof(model->material_name) - 1)
            max_len = sizeof(model->material_name) - 1;
        strncpy(model->material_name, name, max_len);
        model->material_name[sizeof(model->material_name) - 1] = '\0';
    }

    return model;
}

Gfx3dModel *gfx3d_model_load_c3d(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    if (file_size <= 0 || file_size > 16 * 1024 * 1024) {
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_SET);

    uint8_t *buf = malloc(file_size);
    if (!buf) { fclose(f); return NULL; }

    if (fread(buf, 1, file_size, f) != (size_t)file_size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);

    Gfx3dModel *model = gfx3d_model_load_c3d_mem(buf, (uint32_t)file_size);
    free(buf);
    return model;
}

void gfx3d_model_destroy(Gfx3dModel *model) {
    if (!model) return;
    gfx3d_mesh_destroy(model->mesh);
    free(model);
}
