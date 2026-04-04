#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdlib.h>
#include <string.h>

bool st_predeclare_top_level_bindings(SymbolTable *table, const AstProgram *program) {
    size_t i;

    for (i = 0; i < program->top_level_count; i++) {
        const AstTopLevelDecl *decl = program->top_level_decls[i];

        if (decl->kind == AST_TOP_LEVEL_BINDING) {
            const AstBindingDecl *binding_decl = &decl->as.binding_decl;
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  binding_decl->name);
            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, binding_decl->name);
            }
            Symbol *symbol;

            if (conflicting_symbol) {
                st_set_error_at(table,
                                binding_decl->name_span,
                                &conflicting_symbol->declaration_span,
                                "Duplicate symbol '%s' in %s.",
                                binding_decl->name,
                                scope_kind_name(table->root_scope->kind));
                return false;
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_TOP_LEVEL_BINDING,
                                   binding_decl->name, NULL,
                                   &binding_decl->declared_type,
                                   binding_decl->is_inferred_type,
                                   st_top_level_binding_is_final(binding_decl),
                                   ast_decl_has_modifier(binding_decl->modifiers,
                                                         binding_decl->modifier_count,
                                                         AST_MODIFIER_EXPORT),
                                   ast_decl_has_modifier(binding_decl->modifiers,
                                                         binding_decl->modifier_count,
                                                         AST_MODIFIER_STATIC),
                                   ast_decl_has_modifier(binding_decl->modifiers,
                                                         binding_decl->modifier_count,
                                                         AST_MODIFIER_INTERNAL),
                                   binding_decl->name_span,
                                   binding_decl,
                                   table->root_scope);
            if (!symbol) {
                return false;
            }

            if (!st_scope_append_symbol(table, table->root_scope, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_UNION) {
            const AstUnionDecl *union_decl = &decl->as.union_decl;
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  union_decl->name);
            if (!conflicting_symbol) {
                conflicting_symbol = symbol_table_find_import(table, union_decl->name);
            }
            Symbol *symbol;

            if (conflicting_symbol) {
                st_set_error_at(table,
                                union_decl->name_span,
                                &conflicting_symbol->declaration_span,
                                "Duplicate symbol '%s' in %s.",
                                union_decl->name,
                                scope_kind_name(table->root_scope->kind));
                return false;
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_UNION,
                                   union_decl->name, NULL,
                                   NULL, false, false,
                                   ast_decl_has_modifier(union_decl->modifiers,
                                                         union_decl->modifier_count,
                                                         AST_MODIFIER_EXPORT),
                                   ast_decl_has_modifier(union_decl->modifiers,
                                                         union_decl->modifier_count,
                                                         AST_MODIFIER_STATIC),
                                   false,
                                   union_decl->name_span,
                                   union_decl,
                                   table->root_scope);
            if (!symbol) {
                return false;
            }

            symbol->generic_param_count = union_decl->generic_param_count;

            if (!st_scope_append_symbol(table, table->root_scope, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
        } else if (decl->kind == AST_TOP_LEVEL_EXTERN) {
            const AstExternDecl *extern_decl = &decl->as.extern_decl;
            const Symbol *conflicting_symbol = scope_lookup_local(table->root_scope,
                                                                  extern_decl->name);
            Symbol *symbol;

            if (conflicting_symbol) {
                st_set_error_at(table,
                                extern_decl->name_span,
                                &conflicting_symbol->declaration_span,
                                "Duplicate symbol '%s' in %s.",
                                extern_decl->name,
                                scope_kind_name(table->root_scope->kind));
                return false;
            }

            symbol = st_symbol_new(table, SYMBOL_KIND_EXTERN,
                                   extern_decl->name, NULL,
                                   &extern_decl->return_type,
                                   false, false, false, false, false,
                                   extern_decl->name_span,
                                   extern_decl,
                                   table->root_scope);
            if (!symbol) {
                return false;
            }

            if (!st_scope_append_symbol(table, table->root_scope, symbol)) {
                st_symbol_free(symbol);
                return false;
            }
        }
    }

    return true;
}
