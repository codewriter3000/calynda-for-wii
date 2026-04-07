# Calynda for Wii 0.2.1

April 4, 2026

Calynda for Wii 0.2.1 is a refactoring and cleanup release. No new language features are
introduced.

## Highlights

- The obsolete V1 grammar has been removed. The canonical grammar now lives at
  `compiler/calynda.ebnf` and reflects the full V2 language surface.
- `manual()` and `asm()` references have been removed from the grammar. Both are
  deferred to V3 and will not ship until their full design cycle is complete.
- Every compiler source file in `compiler/src/` has been refactored to stay under
  250 lines by splitting large modules into focused compilation units organized
  by logical concern (e.g., `parser_expr.c`, `parser_decl.c`).
- The Makefile has been updated to build all new split files.

## Grammar Cleanup

- Deleted the obsolete V1 grammar (`compiler/calynda.ebnf`).
- Removed `ManualStatement`, `ManualBody`, `MallocStatement`, `CallocStatement`,
  `ReallocStatement`, `FreeStatement`, and pointer operators `*`/`&` from the
  grammar — all deferred to V3.
- Renamed `compiler/calynda_v2.ebnf` to `compiler/calynda.ebnf` as the sole
  canonical grammar.

## Refactoring

- Split 26 source files that exceeded 250 lines into ~65 focused compilation
  units. Each split follows the module's existing header and public API — no
  external interface changes.
- All 1147 tests continue to pass.

---

# Calynda for Wii 0.2.0

April 4, 2026

Calynda for Wii 0.2.0 lands the V2 language surface on top of the 0.1.0 compiled
toolchain.

## Highlights

- Reified generics with Java-style `<T>`, `<K, V>`, and wildcard `<?>` syntax.
- Heterogeneous arrays via `arr<?>`, replacing the old struct concept.
- Tagged unions with `union Name<T> { Variant(T), Empty }` declarations.
- Expanded import model: module aliases (`as`), wildcard imports (`.*`),
  selective imports (`{a, b, c}`), and explicit `export` control.
- Primitive aliases: `byte`, `sbyte`, `short`, `int`, `long`, `ulong`, `uint`,
  `float`, `double` alongside Calynda for Wii-native type names.
- Prefix and postfix `++`/`--` operators.
- Varargs parameters (`Type... name`).
- Discard expression (`_`).
- `internal` modifier for nested helper visibility.
- `static` and `export` modifiers for top-level declarations.

## Deferred To V3

- `manual()` — manual memory boundary with malloc/calloc/realloc/free.
- `asm()` — inline assembly. Depends on `manual()` landing first.
- One-statement callback shorthand — pending explicit auto-lift rule decision.

---

# Calynda for Wii 0.1.0

April 2, 2026

This is the first public edition of Calynda for Wii.

Calynda for Wii 0.1.0 ships the first end-to-end compiled toolchain for the language: parsing, semantic analysis, lowering through multiple IR stages, native code generation for the Nintendo Wii PowerPC (Broadway) processor, a portable bytecode emitter, a runtime layer for dynamic language features, and a command-line compiler that can build `.cal` programs into DOL executables.

## Highlights

- Native compilation now works end to end targeting the Wii's PowerPC (Broadway) processor. `calynda build program.cal` emits PowerPC assembly, links it with the runtime via devkitPPC, and produces a runnable DOL executable.
- The CLI also supports `calynda run`, `calynda asm`, and `calynda bytecode`, so the first edition is usable both as a compiler and as an inspection tool.
- A portable backend now exists alongside the native backend. Calynda for Wii can emit `portable-v1` bytecode from MIR as a compiled backend target.
- The repository now includes `install.sh`, which builds Calynda and installs the CLI together with the required `runtime.o` payload for new users.

## Language And Compiler Surface

This first edition includes the front-end and semantic pipeline needed to compile non-trivial Calynda programs.

- Recursive-descent parsing from the Calynda for Wii grammar in [calynda.ebnf](compiler/calynda.ebnf).
- Source-aware diagnostics for parse, symbol, and type errors.
- A required single `start(string[] args)` program entry point.
- Lambda-based callable model with first-class lambdas and closure lowering.
- Arrays, casts, member access, index access, function calls, assignments, ternaries, template literals, and `throw`.
- Multiple lowering stages: AST, HIR, MIR, LIR, codegen planning, machine emission, PowerPC assembly, and portable bytecode.

## Runtime And Backend Model

Calynda for Wii 0.1.0 is a compiled language release, not an interpreter release.

- Primary backend: PowerPC (Broadway) DOL executables for the Nintendo Wii.
- Secondary backend: `portable-v1` bytecode emission.
- Rejected path: interpreting AST, HIR, MIR, or later IR directly.

The runtime currently provides the helper surface needed for closures, callable dispatch, array literals and indexing, template construction, member access, dynamic casts, indexed and member stores, and thrown values. The PowerPC startup wrapper boxes CLI arguments into Calynda `string[]` values before entering `start`.

## Tooling

- CLI binary: `build/calynda`.
- Install script: [install.sh](/home/codewriter3000/Coding/calynda-lang/install.sh).
- VS Code syntax support is included in [vscode-calynda/README.md](vscode-calynda/README.md) and packaged locally as `calynda-syntax-0.1.0.vsix`.

## Installation

For a local user install:

```sh
sh ./install.sh
```

For a custom prefix:

```sh
sh ./install.sh --prefix /usr/local
```

The installer places the launcher in `bin/` and the real CLI plus `runtime.o` under `lib/calynda/` so the compiler can link generated DOL executables correctly.

## Known Limits In 0.1.0

- Native compilation currently targets the Nintendo Wii PowerPC (Broadway) processor and uses devkitPPC (`powerpc-eabi-gcc`) for final linking.
- Output format is DOL; programs can be tested on the Dolphin emulator or run on real Wii hardware via the Homebrew Channel.
- The bytecode backend currently emits the `portable-v1` program form; this release does not yet ship a bytecode executor.
- Runtime-backed language features are present, but the runtime surface is intentionally narrow and focused on the helpers the compiler already lowers to.
- The bundled editor tooling is syntax highlighting and language configuration, not a full language server.

## First Edition Summary

Calynda for Wii 0.1.0 establishes the project's core direction: a compiled functional systems language targeting the Nintendo Wii, with a real PowerPC backend, a defined runtime boundary, and a second compiled backend path for portability. This release is the foundation release for future work on backend depth, runtime capabilities, and broader tooling.