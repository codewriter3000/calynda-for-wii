#include "type_checker_internal.h"

bool tc_validate_program_start_decls(TypeChecker *checker,
                                     const AstProgram *program) {
    const AstStartDecl *first_start = NULL;
    const AstBootDecl  *first_boot  = NULL;
    size_t i;

    if (!checker || !program) {
        return false;
    }

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (!decl) {
            continue;
        }

        if (decl->kind == AST_TOP_LEVEL_START) {
            if (first_start) {
                tc_set_error_at(checker,
                                decl->as.start_decl.start_span,
                                &first_start->start_span,
                                "Program cannot declare multiple start entry points.");
                return false;
            }
            if (first_boot) {
                tc_set_error_at(checker,
                                decl->as.start_decl.start_span,
                                &first_boot->boot_span,
                                "Program cannot declare both start() and boot() entry points.");
                return false;
            }
            first_start = &decl->as.start_decl;
        } else if (decl->kind == AST_TOP_LEVEL_BOOT) {
            if (first_boot) {
                tc_set_error_at(checker,
                                decl->as.boot_decl.boot_span,
                                &first_boot->boot_span,
                                "Program cannot declare multiple boot entry points.");
                return false;
            }
            if (first_start) {
                tc_set_error_at(checker,
                                decl->as.boot_decl.boot_span,
                                &first_start->start_span,
                                "Program cannot declare both start() and boot() entry points.");
                return false;
            }
            first_boot = &decl->as.boot_decl;
        }
    }

    if (!first_start && !first_boot) {
        tc_set_error(checker,
                     "Program must declare exactly one entry point (start() or boot()).");
        return false;
    }

    return true;
}

const TypeCheckInfo *tc_resolve_symbol_info(TypeChecker *checker,
                                            const Symbol *symbol) {
    TypeCheckSymbolEntry *entry;
    TypeCheckInfo resolved_info;

    if (!checker || !symbol) {
        return NULL;
    }

    entry = tc_ensure_symbol_entry(checker, symbol);
    if (!entry) {
        return NULL;
    }

    if (entry->is_resolved) {
        return &entry->info;
    }

    if (entry->is_resolving) {
        /* Allow self-recursion for bindings whose initializer is a lambda.
         * Return tentative callable info so the recursive call can
         * type-check; the actual return type is filled in once the
         * lambda body finishes. */
        const AstExpression *rec_init = NULL;

        if (symbol->kind == SYMBOL_KIND_TOP_LEVEL_BINDING) {
            const AstBindingDecl *bd =
                (const AstBindingDecl *)symbol->declaration;
            if (bd->initializer &&
                bd->initializer->kind == AST_EXPR_LAMBDA) {
                rec_init = bd->initializer;
            }
        } else if (symbol->kind == SYMBOL_KIND_LOCAL) {
            const AstLocalBindingStatement *lb =
                (const AstLocalBindingStatement *)symbol->declaration;
            if (lb->initializer &&
                lb->initializer->kind == AST_EXPR_LAMBDA) {
                rec_init = lb->initializer;
            }
        }

        if (rec_init) {
            entry->info = tc_type_check_info_make_callable(
                tc_checked_type_external(),
                &rec_init->as.lambda.parameters);
            return &entry->info;
        }

        tc_set_error_at(checker, symbol->declaration_span, NULL,
                        "Circular definition involving '%s'.",
                        symbol->name ? symbol->name : "<anonymous>");
        return NULL;
    }

    entry->is_resolving = true;
    resolved_info = tc_type_check_info_make(tc_checked_type_invalid());

    switch (symbol->kind) {
    case SYMBOL_KIND_PACKAGE:
        resolved_info = tc_type_check_info_make_external_value();
        break;

    case SYMBOL_KIND_IMPORT:
        /* Wildcard-resolved members (name != NULL, is_wildcard_import set)
           are callable bridge functions; bare module imports are values. */
        if (symbol->is_wildcard_import && symbol->name != NULL) {
            resolved_info = tc_type_check_info_make_external_callable();
        } else {
            resolved_info = tc_type_check_info_make_external_value();
        }
        break;

    case SYMBOL_KIND_PARAMETER:
        resolved_info = tc_type_check_info_make(tc_checked_type_from_ast_type(checker,
                                                                              symbol->declared_type));
        if (checker->has_error) {
            entry = tc_ensure_symbol_entry(checker, symbol);
            if (entry) { entry->is_resolving = false; }
            return NULL;
        }
        if (resolved_info.type.kind == CHECKED_TYPE_VOID) {
            tc_set_error_at(checker, symbol->declaration_span, NULL,
                            "Parameter '%s' cannot have type void.",
                            symbol->name ? symbol->name : "<anonymous>");
            entry = tc_ensure_symbol_entry(checker, symbol);
            if (entry) { entry->is_resolving = false; }
            return NULL;
        }
        break;

    case SYMBOL_KIND_TOP_LEVEL_BINDING:
        if (!tc_resolve_binding_symbol(checker,
                                       symbol,
                                       &((const AstBindingDecl *)symbol->declaration)->declared_type,
                                       ((const AstBindingDecl *)symbol->declaration)->is_inferred_type,
                                       ((const AstBindingDecl *)symbol->declaration)->initializer,
                                       &resolved_info)) {
            entry = tc_ensure_symbol_entry(checker, symbol);
            if (entry) { entry->is_resolving = false; }
            return NULL;
        }
        break;

    case SYMBOL_KIND_EXTERN: {
        /* Extern C declaration: treat as a callable with external linkage. */
        CheckedType return_type = tc_checked_type_from_ast_type(checker, symbol->declared_type);
        if (checker->has_error) {
            entry = tc_ensure_symbol_entry(checker, symbol);
            if (entry) { entry->is_resolving = false; }
            return NULL;
        }
        resolved_info = tc_type_check_info_make_external_callable();
        resolved_info.callable_return_type = return_type;
        break;
    }

    case SYMBOL_KIND_LOCAL:
        if (!tc_resolve_binding_symbol(checker,
                                       symbol,
                                       &((const AstLocalBindingStatement *)symbol->declaration)->declared_type,
                                       ((const AstLocalBindingStatement *)symbol->declaration)->is_inferred_type,
                                       ((const AstLocalBindingStatement *)symbol->declaration)->initializer,
                                       &resolved_info)) {
            entry = tc_ensure_symbol_entry(checker, symbol);
            if (entry) { entry->is_resolving = false; }
            return NULL;
        }
        break;

    case SYMBOL_KIND_UNION:
        resolved_info = tc_type_check_info_make(
            tc_checked_type_named(symbol->name, symbol->generic_param_count, 0));
        break;

    case SYMBOL_KIND_TYPE_PARAMETER:
        resolved_info = tc_type_check_info_make(tc_checked_type_type_param(symbol->name));
        break;
    }

    /* Re-fetch entry: symbol_entries may have been reallocated during
       nested symbol resolution above. */
    entry = tc_ensure_symbol_entry(checker, symbol);
    if (!entry) {
        return NULL;
    }
    entry->info = resolved_info;
    entry->is_resolving = false;
    entry->is_resolved = true;
    return &entry->info;
}

bool tc_resolve_binding_symbol(TypeChecker *checker,
                               const Symbol *symbol,
                               const AstType *declared_type,
                               bool is_inferred_type,
                               const AstExpression *initializer,
                               TypeCheckInfo *info) {
    const TypeCheckInfo *initializer_info;
    CheckedType target_type;
    CheckedType source_type;
    char source_text[64];
    char target_text[64];

    if (is_inferred_type) {
        initializer_info = tc_check_expression(checker, initializer);
        if (!initializer_info) {
            return false;
        }

        if (!initializer_info->is_callable &&
            (initializer_info->type.kind == CHECKED_TYPE_VOID ||
             initializer_info->type.kind == CHECKED_TYPE_NULL)) {
            tc_set_error_at(checker,
                            initializer ? initializer->source_span : symbol->declaration_span,
                            &symbol->declaration_span,
                            "Cannot infer a type for %s '%s' from %s.",
                            symbol_kind_name(symbol->kind),
                            symbol->name ? symbol->name : "<anonymous>",
                            initializer_info->type.kind == CHECKED_TYPE_VOID
                                ? "a void expression"
                                : "null");
            return false;
        }

        *info = *initializer_info;
        return true;
    }

    target_type = tc_checked_type_from_ast_type(checker, declared_type);
    if (checker->has_error) {
        return false;
    }

    if (initializer && initializer->kind == AST_EXPR_LAMBDA) {
        initializer_info = tc_check_lambda_expression(checker,
                                                      initializer,
                                                      &target_type,
                                                      &symbol->declaration_span);
    } else if (tc_checked_type_is_hetero_array(target_type) &&
               initializer && initializer->kind == AST_EXPR_ARRAY_LITERAL) {
        initializer_info = tc_check_hetero_array_literal(checker, initializer);
    } else {
        initializer_info = tc_check_expression(checker, initializer);
    }
    if (!initializer_info) {
        return false;
    }

    if (target_type.kind == CHECKED_TYPE_VOID && !initializer_info->is_callable) {
        tc_set_error_at(checker, symbol->declaration_span, NULL,
                        "%s '%s' cannot have type void unless initialized with a callable expression.",
                        symbol_kind_name(symbol->kind),
                        symbol->name ? symbol->name : "<anonymous>");
        return false;
    }

    source_type = tc_type_check_source_type(initializer_info);
    if (!(initializer_info->is_callable && source_type.kind == CHECKED_TYPE_EXTERNAL) &&
        !tc_checked_type_assignable(target_type, source_type)) {
        checked_type_to_string(source_type, source_text, sizeof(source_text));
        checked_type_to_string(target_type, target_text, sizeof(target_text));
        tc_set_error_at(checker,
                        initializer ? initializer->source_span : symbol->declaration_span,
                        &symbol->declaration_span,
                        "Cannot assign expression of type %s to %s '%s' of type %s.",
                        source_text,
                        symbol_kind_name(symbol->kind),
                        symbol->name ? symbol->name : "<anonymous>",
                        target_text);
        return false;
    }

    *info = *initializer_info;
    info->type = target_type;
    if (initializer_info->is_callable) {
        info->callable_return_type = target_type;
    }

    return true;
}
