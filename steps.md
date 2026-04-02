[X] Build a recursive-descent parser for the EBNF grammar, with precedence-based expression parsing and decent error reporting.
[X] Add AST pretty-printing or dumping so parser output can be inspected and tested quickly.
[X] Implement symbol tables and scope management for packages, imports, top-level bindings, parameters, and locals.
    [X] Add source-location diagnostics for unresolved identifiers and duplicate-symbol errors.
        [X] Thread source tokens or source spans from the parser into AST declarations and identifier expressions.
        [X] Store source spans on symbols and unresolved-name records so semantic errors can point at both use and declaration sites.
        [X] Standardize semantic error formatting to match parser errors with line/column context.
    [X] Build type checking on top of the resolved symbol information.
        [X] Define a semantic type model that can represent primitives, arrays, void, and any future callable/function types.
        [X] Infer expression types bottom-up and attach them to AST nodes or semantic side tables.
        [X] Check operator compatibility for unary, binary, ternary, cast, call, index, and member expressions.
        [X] Validate implicit vs explicit conversions and reject lossy or illegal assignments.
    [X] Add a semantic dump or inspection tool if you want to debug scopes the same way you already debug the AST.
        [X] Dump scopes hierarchically with declared symbols, kinds, and inferred/declared types.
        [X] Include identifier resolution edges so unresolved names and shadowing are easy to inspect.
        [X] Add focused tests so the dump remains stable enough for quick regression checks.
[X] Add semantic passes: type resolution, type checking, return/exit rules, assignment checks, and validation of start.
    [X] Type resolution.
        [X] Resolve declared types on bindings, parameters, locals, casts, and array dimensions.
        [X] Reject invalid uses of `void` and malformed array declarations.
    [X] Type checking.
        [X] Compute expression result types for literals, identifiers, lambdas, calls, indexing, members, and arrays.
        [X] Enforce operand compatibility and result typing rules for all operators.
    [X] Return and exit rules.
        [X] Validate which contexts permit `return` and `exit`.
        [X] Ensure expression-bodied and block-bodied lambdas satisfy their expected return behavior.
    [X] Assignment checks.
        [X] Ensure assignment targets are assignable l-values.
        [X] Reject writes to `final` bindings and incompatible compound assignments.
    [X] Start validation.
        [X] Require exactly one `start` entry point.
        [X] Validate the `start` parameter and return conventions that the language intends to support.
        [X] Reject top-level programs that are missing or ambiguously define the entry point.
[ ] Define an IR for evaluation/codegen. For a small language, a simple linear or SSA-like IR is enough.
    [ ] Pick the first IR shape: stack machine, three-address code, or SSA-like basic blocks.
    [ ] Define IR values, temporaries, constants, labels, and control-flow terminators.
    [ ] Define how functions/start/lambdas/blocks lower into IR units.
    [ ] Add an IR printer or dumper for debugging and golden tests.
    [ ] Lower a minimal slice first: literals, arithmetic, locals, calls, branches, and returns.
[ ] Choose execution strategy: interpreter first for speed of development, or LLVM/C transpilation/native codegen if performance matters earlier.
    [ ] Decide whether the first milestone is fast validation or long-term codegen flexibility.
    [ ] If interpreter-first, execute directly from AST or IR and define runtime value representation.
    [ ] If transpilation/codegen-first, choose the backend target and its toolchain constraints.
    [ ] Write down the decision so later passes and runtime work target one concrete path.
[ ] Add runtime pieces only when the language semantics demand them: calling convention, memory management hooks, built-in library boundary, and startup/entry handling.
    [ ] Define the runtime surface area needed by the chosen execution strategy.
    [ ] Specify calling conventions for `start`, user functions, and lambdas.
    [ ] Decide how strings, arrays, and template-literal results are represented at runtime.
    [ ] Define the boundary for built-in packages/functions vs user-defined code.
    [ ] Add startup/entry glue only after the entry-point rules and execution model are fixed.