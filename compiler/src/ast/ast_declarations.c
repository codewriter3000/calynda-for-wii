#include "ast_internal.h"

void ast_program_init(AstProgram *program) {
    if (!program) {
        return;
    }
    memset(program, 0, sizeof(*program));
}

void ast_program_free(AstProgram *program) {
    size_t i;

    if (!program) {
        return;
    }

    ast_qualified_name_free(&program->package_name);

    for (i = 0; i < program->import_count; i++) {
        ast_import_decl_free(&program->imports[i]);
    }
    free(program->imports);

    for (i = 0; i < program->top_level_count; i++) {
        ast_top_level_decl_free(program->top_level_decls[i]);
    }
    free(program->top_level_decls);

    memset(program, 0, sizeof(*program));
}

bool ast_program_set_package(AstProgram *program, AstQualifiedName *package_name) {
    if (!program || !package_name) {
        return false;
    }

    ast_qualified_name_free(&program->package_name);
    program->package_name = *package_name;
    program->has_package = true;
    ast_qualified_name_init(package_name);
    return true;
}

bool ast_program_add_import(AstProgram *program, AstImportDecl *import_decl) {
    if (!program || !import_decl) {
        return false;
    }

    if (!ast_reserve_items((void **)&program->imports, &program->import_capacity,
                           program->import_count + 1, sizeof(*program->imports))) {
        return false;
    }

    program->imports[program->import_count++] = *import_decl;
    memset(import_decl, 0, sizeof(*import_decl));
    return true;
}

bool ast_program_add_top_level_decl(AstProgram *program, AstTopLevelDecl *decl) {
    if (!program || !decl) {
        return false;
    }

    if (!ast_reserve_items((void **)&program->top_level_decls,
                           &program->top_level_capacity,
                           program->top_level_count + 1,
                           sizeof(*program->top_level_decls))) {
        return false;
    }

    program->top_level_decls[program->top_level_count++] = decl;
    return true;
}

void ast_import_decl_init(AstImportDecl *decl) {
    if (!decl) {
        return;
    }
    memset(decl, 0, sizeof(*decl));
}

void ast_import_decl_free(AstImportDecl *decl) {
    size_t i;

    if (!decl) {
        return;
    }

    ast_qualified_name_free(&decl->module_name);
    free(decl->alias);

    for (i = 0; i < decl->selected_count; i++) {
        free(decl->selected_names[i]);
    }
    free(decl->selected_names);

    memset(decl, 0, sizeof(*decl));
}

bool ast_import_decl_add_selected(AstImportDecl *decl, const char *name) {
    char *copy;

    if (!decl || !name) {
        return false;
    }

    copy = ast_copy_text(name);
    if (!copy) {
        return false;
    }

    if (!ast_reserve_items((void **)&decl->selected_names, &decl->selected_capacity,
                           decl->selected_count + 1, sizeof(*decl->selected_names))) {
        free(copy);
        return false;
    }

    decl->selected_names[decl->selected_count++] = copy;
    return true;
}

void ast_union_decl_free_fields(AstUnionDecl *decl) {
    size_t i;

    if (!decl) {
        return;
    }

    free(decl->modifiers);
    free(decl->name);

    for (i = 0; i < decl->generic_param_count; i++) {
        free(decl->generic_params[i]);
    }
    free(decl->generic_params);

    for (i = 0; i < decl->variant_count; i++) {
        free(decl->variants[i].name);
        if (decl->variants[i].payload_type) {
            ast_type_free(decl->variants[i].payload_type);
            free(decl->variants[i].payload_type);
        }
    }
    free(decl->variants);

    memset(decl, 0, sizeof(*decl));
}

bool ast_union_decl_add_generic_param(AstUnionDecl *decl, const char *param) {
    char *copy;

    if (!decl || !param) {
        return false;
    }

    copy = ast_copy_text(param);
    if (!copy) {
        return false;
    }

    if (!ast_reserve_items((void **)&decl->generic_params, &decl->generic_param_capacity,
                           decl->generic_param_count + 1, sizeof(*decl->generic_params))) {
        free(copy);
        return false;
    }

    decl->generic_params[decl->generic_param_count++] = copy;
    return true;
}

bool ast_union_decl_add_variant(AstUnionDecl *decl, AstUnionVariant *variant) {
    if (!decl || !variant) {
        return false;
    }

    if (!ast_reserve_items((void **)&decl->variants, &decl->variant_capacity,
                           decl->variant_count + 1, sizeof(*decl->variants))) {
        return false;
    }

    decl->variants[decl->variant_count++] = *variant;
    memset(variant, 0, sizeof(*variant));
    return true;
}
