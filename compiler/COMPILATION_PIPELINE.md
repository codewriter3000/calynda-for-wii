# Calynda Compilation Pipeline

This document describes how a `.cal` source file becomes a PowerPC executable
(`.dol`) that runs on the Wii.

---

## Overview

```
source.cal
    │
    ▼
┌─────────────┐
│  Tokenizer  │  calynda_fork.ebnf tokens → Token stream
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Parser    │  Token stream → AST (AstProgram)
└──────┬──────┘
       │
       ▼
┌──────────────────┐
│  Symbol Table    │  AST → scoped symbol definitions
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│  Type Checker    │  symbols + AST → CheckedType per expression/symbol
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│  HIR Lowering    │  AST + symbols + types → HirProgram
└──────┬───────────┘
       │
       ▼
┌──────────────────┐
│   C Emitter      │  HirProgram → generated C translation unit (.c)
└──────┬───────────┘
       │
       ▼
┌────────────────────────┐
│  powerpc-eabi-g++      │  generated C + runtime libs → ELF
│  (DevkitPPC cross CC)  │
└──────┬─────────────────┘
       │
       ▼
┌─────────────┐
│  elf2dol    │  ELF → DOL (Wii executable format)
└─────────────┘
```

For host (Linux/macOS) builds the last two steps are replaced by a plain `gcc`
invocation that produces a native executable.  For JSX component sources a
separate fast path skips sema/HIR and emits C directly.

---

## Stage 1 — Tokenizer (`src/tokenizer/`)

**Entry:** `parser_init(&parser, source)` (the parser owns the tokenizer)
**Output:** stream of `Token` values consumed lazily by the parser

The tokenizer converts the raw source string into a flat stream of typed tokens.
It is a hand-written single-pass scanner split across four files:

| File | Responsibility |
|------|---------------|
| `tokenizer.c` | Core state machine and public API |
| `tokenizer_keywords.c` | Keyword recognition (`start`, `boot`, `extern`, `var`, …) |
| `tokenizer_literals.c` | Numeric and string literals, template strings |
| `tokenizer_punctuation.c` | Operators and punctuation |

Key token categories are defined in `tokenizer.h`: literals (`TOK_INT_LIT`,
`TOK_FLOAT_LIT`, `TOK_STRING_LIT`, template parts), the keyword set, identifiers,
and all operator/punctuation tokens.

64-bit types (`int64`, `uint64`, `float64`) are not lexed as keywords in this
fork — the Wii's `CalyndaRtWord` is 32-bit.

---

## Stage 2 — Parser (`src/parser/`)

**Input:** token stream
**Output:** `AstProgram` — a tree of `AstTopLevelDecl` nodes

A recursive-descent parser (no parser generator) structured around the grammar
in [calynda_fork.ebnf](calynda_fork.ebnf).  Files map to grammar concerns:

| File | Responsibility |
|------|---------------|
| `parser.c` | Top-level program parse, init/free |
| `parser_decl.c` | `start`, `boot`, `extern`, binding declarations |
| `parser_union.c` | `union` type declarations |
| `parser_stmt.c` | Statements (`var`, `return`, `exit`, `throw`, …) |
| `parser_expr.c` | Primary expressions |
| `parser_binary.c` | Binary operator precedence climbing |
| `parser_postfix.c` | Call, index, member access, optional chaining |
| `parser_literals.c` | Literal nodes |
| `parser_types.c` | Type annotation parsing |
| `parser_lookahead.c` | k-token lookahead helpers |
| `parser_utils.c` | Error recording, token-consume utilities |

`calynda_jsx_parser.c` handles the JSX fast-path (component declarations).

---

## Stage 3 — Symbol Table (`src/sema/`)

**Input:** `AstProgram`
**Output:** `SymbolTable` — a tree of `Scope` objects, each holding `Symbol` entries

`symbol_table_build()` walks the AST in two passes:

1. **Pre-declaration pass** (`symbol_table_predecl.c`) — registers top-level
   names so forward references resolve.
2. **Analysis pass** (`symbol_table_analyze.c` / `symbol_table_analyze_expr.c`)
   — resolves every identifier to a `Symbol`, builds child scopes for
   `start`, `boot`, lambda, and block bodies.

`Symbol` carries: kind (`SYMBOL_KIND_TOP_LEVEL_BINDING`, `SYMBOL_KIND_LOCAL`,
`SYMBOL_KIND_EXTERN`, `SYMBOL_KIND_UNION`, …), declared type, visibility flags
(`is_exported`, `is_static`, `is_internal`), and a back-pointer to its `Scope`.

`symbol_table_imports.c` resolves `import game.gfx;` style declarations and
wires them to the bridge package objects registered in `calynda_runtime.c`.

---

## Stage 4 — Type Checker (`src/sema/`)

**Input:** `AstProgram` + `SymbolTable`
**Output:** `TypeChecker` — maps every `AstExpression` and `Symbol` to a
`CheckedType` + `TypeCheckInfo`

`type_checker_check_program()` infers and verifies types:

- `type_checker_expr.c` / `_expr_ext.c` / `_expr_more.c` — expression typing
- `type_checker_ops.c` — operator result types and coercion rules
- `type_checker_lambda.c` — lambda / closure parameter and return inference
- `type_checker_block.c` — block-level control flow and binding type propagation
- `type_checker_types.c` — named type and union variant lookup
- `type_checker_convert.c` — implicit conversion validity
- `type_checker_resolve.c` — deferred type resolution for recursive definitions

`CheckedTypeKind` values: `CHECKED_TYPE_VOID`, `CHECKED_TYPE_VALUE` (primitive),
`CHECKED_TYPE_NAMED` (union/generic), `CHECKED_TYPE_EXTERNAL` (extern C),
`CHECKED_TYPE_TYPE_PARAM` (generic parameter).

`TypeResolver` (`type_resolution.c`) caches `AstType → ResolvedType` mappings
used by both the type checker and HIR lowering.

---

## Stage 5 — HIR Lowering (`src/hir/`)

**Input:** `AstProgram` + `SymbolTable` + `TypeChecker`
**Output:** `HirProgram` — a fully typed, scope-resolved intermediate
representation

`hir_build_program()` converts the raw AST into the HIR, which is the last
representation before code generation.  Key properties:

- All identifier references are resolved to `Symbol *` pointers — no further
  name lookup required.
- Every expression and binding carries a `CheckedType`.
- Closures capture their free variables explicitly via `HirClosureCapture` lists.
- Generic union variants record their `CheckedType` payload.
- `start` / `boot` bodies become `HirStartDecl` / `HirBootDecl` with typed
  `HirParameterList` and an `HirBlock`.

HIR node hierarchy:

```
HirProgram
  ├── HirTopLevelDecl[]        (binding / start / boot / union / extern)
  │     ├── HirBindingDecl     (global variables and lambdas)
  │     ├── HirStartDecl       (program entry point)
  │     ├── HirBootDecl        (freestanding entry point)
  │     ├── HirUnionDecl       (sum type)
  │     └── HirExternDecl      (C FFI)
  └── HirNamedSymbol[]         (resolved imports)
```

Each `HirBlock` is a flat list of `HirStatement` (local binding, return, exit,
throw, or expression).  `HirExpression` is a tagged union covering every
expression form in the language.

---

## Stage 6 — C Emitter (`src/c_emit/`)

**Input:** `HirProgram`
**Output:** a C translation unit written to a `FILE *`

`c_emit_program_with_file()` serialises the HIR to portable C that the
downstream compiler can consume.  The generated file:

1. **File header** — `#include`s for standard headers and `calynda_runtime.h`;
   inline helpers `__calynda_float_to_word` / `__calynda_word_to_float` for
   the `CalyndaRtWord ↔ float` union cast.
2. **Import macros** — `#define __cal_gfx ((CalyndaRtWord)&__calynda_pkg_gfx)`
   etc., one per `import` statement.
3. **Forward declarations** — `extern` C prototypes, `static CalyndaRtWord`
   globals, and forward declarations for `start`/`boot` functions.
4. **Global initialisers** — values for top-level `var` bindings.
5. **Function bodies** — one C function per lambda/start/boot block.
   - `c_emit_stmt.c` lowers `HirStatement` nodes.
   - `c_emit_expr.c` lowers `HirExpression` nodes.
   - `c_emit_closure.c` emits closure struct types and factory functions.
   - `c_emit_type.c` maps `CheckedType` to C type strings.
   - `c_emit_names.c` mangles Calynda identifiers to safe C names.
6. **`calynda_program_main()`** — the C function that the Wii wrapper (or host
   `main`) calls; invokes `calynda_rt_start_process()` with the `start` entry.

`#line` directives are emitted so compiler errors reference the original `.cal`
file instead of the generated C.

`calynda_jsx_emit.c` / `c_emit_jsx_file.c` handle JSX component sources on the
fast path.

---

## Stage 7 — C Compilation (`powerpc-eabi-g++`)

**Input:** generated `.c` + pre-built runtime libraries
**Output:** ELF executable

`calynda_run_c_compiler()` in `calynda_utils.c` forks a child process and
`execvp`s the cross compiler with:

```
powerpc-eabi-g++ -DCALYNDA_WII_BUILD -x c
    -o <output>.elf
    <generated>.c
    -I<build>/wii           # calynda_runtime.h, bridge headers
    -I$DEVKITPRO/libogc/include
    -L<build>/wii           # libcalynda_runtime.a
    -L$DEVKITPRO/libogc/lib/wii
    -L$DEVKITPRO/portlibs/ppc/lib
    -lsolite_wii -lcalynda_runtime -lwiigui
    -lfreetype -lpng -lz -lbz2 -lbrotlidec -lbrotlicommon
    -lvorbisidec -logg -lasnd
    -lwiiuse -lbte -logc -lm
    -mrvl -mcpu=750 -meabi -mhard-float
```

`-DCALYNDA_WII_BUILD` enables real GX/libogc calls inside the bridge and game
engine libraries; without it the same sources compile against host stubs for
unit testing.

`-mcpu=750` targets the Broadway (PowerPC 750CL) CPU.  `-mrvl` enables
Revolution (Wii) ABI.  `-mhard-float` uses the FPU.

The runtime library (`libcalynda_runtime.a`) contains:

| Object | Purpose |
|--------|---------|
| `calynda_runtime.o` | Core word/object/GC operations, `stdlib` package |
| `calynda_gc_refcount.o` | Reference-counting GC |
| `calynda_wii_main.o` | `main()` that calls `calynda_program_main()` after `calynda_wii_console_init()` |
| `calynda_wii_io.o` | libogc VIDEO + console initialisation |
| `calynda_gfx_bridge.o` | `import game.gfx;` dispatch (→ `calynda_gfx3d`) |
| `calynda_physics_bridge.o` | `import game.physics;` dispatch (→ `calynda_physics`) |
| `calynda_motion_bridge.o` | `import game.motion;` dispatch (→ `calynda_motion`) |
| `calynda_wpad.o` | Wii Remote input wrappers |
| `calynda_mii.o` / `mii.o` | Mii data helpers |
| `calynda_math.o` | Math library (`CmVec3`, `CmMat4`, …) |
| `gfx3d_*.o` (×12) | 3D graphics engine (GX rendering) |
| `calynda_physics.o` | Physics engine |
| `calynda_motion.o` | MotionPlus fusion + gesture detection |

For **host** builds, `gcc` is used instead and only `libcalynda_runtime` +
`libsolite_wii_host` are linked (no GX, no libogc).

---

## Stage 8 — ELF → DOL Conversion

**Input:** `<output>.elf`
**Output:** `<output>.dol`

`calynda_run_elf2dol()` invokes the `elf2dol` tool from DevkitPPC.  The DOL
format is the native executable format for GameCube and Wii; Dolphin emulator
and the Homebrew Channel both load `.dol` files directly.

---

## CLI Entry Points

```
calynda build  source.cal [-o output] [--target host|wii|gc]
calynda emit-c source.cal          # print generated C to stdout (debug)
calynda run    source.cal [args…]  # build to /tmp and execute immediately
```

The `run` subcommand always uses the host target and deletes the temporary
executable after the child process exits.

---

## JSX Fast Path

If the source contains `component` declarations, `jsx_source_has_components()`
returns true and `jsx_compile_to_c()` is called directly, bypassing the
symbol table, type checker, and HIR entirely.  This path lives in
`c_emit/c_emit_jsx_file.c` and `c_emit/calynda_jsx_emit.c`.

---

## Data Flow Summary

```
.cal source (text)
  ──tokenizer──▶  Token[]
  ──parser──────▶  AstProgram
  ──symbol_table──▶  SymbolTable  (Scope tree, Symbol map)
  ──type_checker──▶  TypeChecker  (CheckedType per expression)
  ──hir_lower────▶  HirProgram   (typed, resolved IR)
  ──c_emit───────▶  generated .c
  ──powerpc-eabi-g++──▶  .elf
  ──elf2dol──────▶  .dol  (runs on Wii / Dolphin)
```
