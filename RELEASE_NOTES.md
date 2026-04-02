# Calynda 0.1.0

April 2, 2026

This is the first public edition of Calynda.

Calynda 0.1.0 ships the first end-to-end compiled toolchain for the language: parsing, semantic analysis, lowering through multiple IR stages, native code generation for Linux x86_64, a portable bytecode emitter, a runtime layer for dynamic language features, and a command-line compiler that can build and run `.cal` programs.

## Highlights

- Native compilation now works end to end on Linux x86_64 SysV ELF. `calynda build program.cal` emits native assembly, links it with the runtime, and produces a runnable executable.
- The CLI also supports `calynda run`, `calynda asm`, and `calynda bytecode`, so the first edition is usable both as a compiler and as an inspection tool.
- A portable backend now exists alongside the native backend. Calynda can emit `portable-v1` bytecode from MIR as a compiled backend target.
- The repository now includes `install.sh`, which builds Calynda and installs the CLI together with the required `runtime.o` payload for new users.

## Language And Compiler Surface

This first edition includes the front-end and semantic pipeline needed to compile non-trivial Calynda programs.

- Recursive-descent parsing from the Calynda grammar in [calynda.ebnf](/home/codewriter3000/Coding/calynda-lang/calynda.ebnf).
- Source-aware diagnostics for parse, symbol, and type errors.
- A required single `start(string[] args)` program entry point.
- Lambda-based callable model with first-class lambdas and closure lowering.
- Arrays, casts, member access, index access, function calls, assignments, ternaries, template literals, and `throw`.
- Multiple lowering stages: AST, HIR, MIR, LIR, codegen planning, machine emission, native assembly, and portable bytecode.

## Runtime And Backend Model

Calynda 0.1.0 is a compiled language release, not an interpreter release.

- Primary backend: native x86_64 SysV ELF.
- Secondary backend: `portable-v1` bytecode emission.
- Rejected path: interpreting AST, HIR, MIR, or later IR directly.

The runtime currently provides the helper surface needed for closures, callable dispatch, array literals and indexing, template construction, member access, dynamic casts, indexed and member stores, and thrown values. Native startup also boxes CLI arguments into Calynda `string[]` values before entering `start`.

## Tooling

- CLI binary: `build/calynda`.
- Install script: [install.sh](/home/codewriter3000/Coding/calynda-lang/install.sh).
- VS Code syntax support is included in [vscode-calynda/README.md](/home/codewriter3000/Coding/calynda-lang/vscode-calynda/README.md) and packaged locally as `calynda-syntax-0.1.0.vsix`.

## Installation

For a local user install:

```sh
sh ./install.sh
```

For a custom prefix:

```sh
sh ./install.sh --prefix /usr/local
```

The installer places the launcher in `bin/` and the real CLI plus `runtime.o` under `lib/calynda/` so the compiler can link generated executables correctly.

## Known Limits In 0.1.0

- Native compilation currently targets Linux x86_64 SysV ELF and uses `gcc` for final linking.
- The bytecode backend currently emits the `portable-v1` program form; this release does not yet ship a bytecode executor.
- Runtime-backed language features are present, but the runtime surface is intentionally narrow and focused on the helpers the compiler already lowers to.
- The bundled editor tooling is syntax highlighting and language configuration, not a full language server.

## First Edition Summary

Calynda 0.1.0 establishes the project's core direction: a compiled functional systems language with a real native backend, a defined runtime boundary, and a second compiled backend path for portability. This release is the foundation release for future work on backend depth, runtime capabilities, and broader tooling.