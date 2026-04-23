#include "symbol_table.h"
#include "symbol_table_internal.h"

#include <stdlib.h>
#include <string.h>

/* Try to resolve identifier from a wildcard import.  Returns a newly
   created synthetic SYMBOL_KIND_IMPORT symbol on success, or NULL if no
   wildcard import covers it.  The returned symbol is owned by the table's
   imports list. */
static const Symbol *st_try_wildcard_resolve(SymbolTable *table,
                                              Scope *scope,
                                              const char *identifier) {
    size_t i;
    const char *dot;
    char *module_alias;

    for (i = 0; i < table->import_count; i++) {
        Symbol *sentinel = table->imports[i];
        Symbol *syn;

        if (!sentinel || !sentinel->is_wildcard_import) {
            continue;
        }

        /* Extract the last path segment (module alias) as qualified_name
           for the synthetic symbol so the HIR can emit __cal_<alias>. */
        dot = strrchr(sentinel->qualified_name, '.');
        module_alias = dot ? ast_copy_text(dot + 1)
                           : ast_copy_text(sentinel->qualified_name);
        if (!module_alias) {
            return NULL;
        }

        syn = st_symbol_new(table, SYMBOL_KIND_IMPORT,
                            identifier, module_alias,
                            NULL, false, false, false, false, false,
                            sentinel->declaration_span, sentinel->declaration,
                            scope);
        free(module_alias);
        if (!syn) {
            return NULL;
        }
        syn->is_wildcard_import = true;
        if (!st_imports_append(table, syn)) {
            st_symbol_free(syn);
            return NULL;
        }
        return syn;
    }
    return NULL;
}

bool st_analyze_expression(SymbolTable *table, const AstExpression *expression,
                           Scope *scope) {
    size_t i;

    if (!expression) {
        return true;
    }

    switch (expression->kind) {
    case AST_EXPR_LITERAL:
        if (expression->as.literal.kind == AST_LITERAL_TEMPLATE) {
            for (i = 0; i < expression->as.literal.as.template_parts.count; i++) {
                const AstTemplatePart *part =
                    &expression->as.literal.as.template_parts.items[i];

                if (part->kind == AST_TEMPLATE_PART_EXPRESSION &&
                    !st_analyze_expression(table, part->as.expression, scope)) {
                    return false;
                }
            }
        }
        return true;

    case AST_EXPR_IDENTIFIER:
        {
            const Symbol *resolved = symbol_table_lookup(table, scope,
                                                         expression->as.identifier);

            if (!resolved) {
                resolved = st_try_wildcard_resolve(table, scope,
                                                   expression->as.identifier);
            }

            if (resolved) {
                return st_resolutions_append(table, expression, scope, resolved);
            }

            return st_unresolved_append(table, expression, scope);
        }

    case AST_EXPR_LAMBDA:
        return st_analyze_lambda_expression(table, expression, scope);

    case AST_EXPR_ASSIGNMENT:
        return st_analyze_expression(table, expression->as.assignment.target, scope) &&
               st_analyze_expression(table, expression->as.assignment.value, scope);

    case AST_EXPR_TERNARY:
        return st_analyze_expression(table, expression->as.ternary.condition, scope) &&
               st_analyze_expression(table, expression->as.ternary.then_branch, scope) &&
               st_analyze_expression(table, expression->as.ternary.else_branch, scope);

    case AST_EXPR_BINARY:
        return st_analyze_expression(table, expression->as.binary.left, scope) &&
               st_analyze_expression(table, expression->as.binary.right, scope);

    case AST_EXPR_UNARY:
        return st_analyze_expression(table, expression->as.unary.operand, scope);

    case AST_EXPR_CALL:
        if (!st_analyze_expression(table, expression->as.call.callee, scope)) {
            return false;
        }
        for (i = 0; i < expression->as.call.arguments.count; i++) {
            if (!st_analyze_expression(table, expression->as.call.arguments.items[i], scope)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_INDEX:
        return st_analyze_expression(table, expression->as.index.target, scope) &&
               st_analyze_expression(table, expression->as.index.index, scope);

    case AST_EXPR_MEMBER:
        return st_analyze_expression(table, expression->as.member.target, scope);

    case AST_EXPR_CAST:
        return st_analyze_expression(table, expression->as.cast.expression, scope);

    case AST_EXPR_ARRAY_LITERAL:
        for (i = 0; i < expression->as.array_literal.elements.count; i++) {
            if (!st_analyze_expression(table,
                                       expression->as.array_literal.elements.items[i],
                                       scope)) {
                return false;
            }
        }
        return true;

    case AST_EXPR_GROUPING:
        return st_analyze_expression(table, expression->as.grouping.inner, scope);

    case AST_EXPR_DISCARD:
        return true;

    case AST_EXPR_POST_INCREMENT:
        return st_analyze_expression(table, expression->as.post_increment.operand, scope);

    case AST_EXPR_POST_DECREMENT:
        return st_analyze_expression(table, expression->as.post_decrement.operand, scope);
    }

    return false;
}

bool st_add_parameter_symbols(SymbolTable *table,
                              const AstParameterList *parameters,
                              Scope *scope) {
    size_t i;

    for (i = 0; i < parameters->count; i++) {
        const AstParameter *parameter = &parameters->items[i];
        const Symbol *conflicting_symbol = scope_lookup_local(scope, parameter->name);
        Symbol *symbol;

        if (conflicting_symbol) {
            st_set_error_at(table,
                            parameter->name_span,
                            &conflicting_symbol->declaration_span,
                            "Duplicate symbol '%s' in %s.",
                            parameter->name,
                            scope_kind_name(scope->kind));
            return false;
        }

        symbol = st_symbol_new(table, SYMBOL_KIND_PARAMETER,
                               parameter->name, NULL,
                               &parameter->type,
                               false, false,
                               false, false, false,
                               parameter->name_span,
                               parameter, scope);
        if (!symbol) {
            return false;
        }

        if (!st_scope_append_symbol(table, scope, symbol)) {
            st_symbol_free(symbol);
            return false;
        }
    }

    return true;
}

bool st_add_local_symbol(SymbolTable *table,
                         const AstLocalBindingStatement *binding,
                         Scope *scope) {
    const Symbol *conflicting_symbol = scope_lookup_local(scope, binding->name);
    Symbol *symbol;

    if (conflicting_symbol) {
        st_set_error_at(table,
                        binding->name_span,
                        &conflicting_symbol->declaration_span,
                        "Duplicate symbol '%s' in %s.",
                        binding->name,
                        scope_kind_name(scope->kind));
        return false;
    }

    symbol = st_symbol_new(table, SYMBOL_KIND_LOCAL,
                           binding->name, NULL,
                           &binding->declared_type,
                           binding->is_inferred_type,
                           binding->is_final,
                           false, false,
                           binding->is_internal,
                           binding->name_span,
                           binding, scope);
    if (!symbol) {
        return false;
    }

    if (!st_scope_append_symbol(table, scope, symbol)) {
        st_symbol_free(symbol);
        return false;
    }

    return true;
}
