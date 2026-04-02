# Calynda

Calynda is a compiled functional systems programming language.

The first public edition ships an end-to-end compiler pipeline, a native Linux x86_64 backend, a portable bytecode emitter, a runtime layer for dynamic language features, and a command-line tool that can build and run `.cal` programs.

## Status

Calynda is currently at 0.1.0.

What is in this first edition:

- Recursive-descent parsing and source-aware diagnostics.
- Semantic analysis with symbol resolution and type checking.
- Lowering through AST, HIR, MIR, LIR, codegen planning, machine emission, and assembly emission.
- Native compilation for Linux x86_64 SysV ELF.
- Portable `portable-v1` bytecode emission.
- A small VS Code syntax extension in [vscode-calynda/README.md](/home/codewriter3000/Coding/calynda-lang/vscode-calynda/README.md).

What is not in this edition:

- No interpreter path.
- No bytecode executor yet.
- No full language server yet.

More detail is in [RELEASE_NOTES.md](/home/codewriter3000/Coding/calynda-lang/RELEASE_NOTES.md).

## Installation

For a normal user install:

```sh
sh ./install.sh
```

For a custom prefix:

```sh
sh ./install.sh --prefix /usr/local
```

The installer builds Calynda and installs:

- a `calynda` launcher in your bin directory
- the real CLI binary under `lib/calynda/`
- the required `runtime.o` beside that binary so generated executables can link correctly

If your install bin directory is not on `PATH`, the installer prints the export line to add to your shell profile.

## Build From Source

Requirements:

- `gcc`
- `make`
- standard Unix userland tools used by the build and install scripts

To build the CLI locally:

```sh
make calynda
```

The resulting compiler binary is:

```text
build/calynda
```

## Quick Start

Create a file named `hello.cal`:

```cal
import io.stdlib;

start(string[] args) -> {
    var render = (string name) -> `hello ${name}`;
    stdlib.print(render(args[0]));
    return 0;
};
```

Build it:

```sh
calynda build hello.cal
```

Run it:

```sh
./build/hello world
```

Or compile and run in one step:

```sh
calynda run hello.cal world
```

## CLI

The current CLI commands are:

```text
calynda build <source.cal> [-o output]
calynda run <source.cal> [args...]
calynda asm <source.cal>
calynda bytecode <source.cal>
calynda help
```

What they do:

- `build`: compile a source file to a native executable
- `run`: build a temporary native executable and execute it
- `asm`: emit native x86_64 assembly to stdout
- `bytecode`: emit `portable-v1` bytecode text to stdout

## Language Snapshot

The grammar lives in [calynda.ebnf](/home/codewriter3000/Coding/calynda-lang/calynda.ebnf).

Current language surface includes:

- a required single `start(string[] args)` entry point
- first-class lambdas with arrow syntax
- arrays
- function calls
- member access and index access
- casts
- assignments
- ternary expressions
- template literals with interpolation
- `throw`

The project is intentionally compiled-only right now. Native code and bytecode are the supported backend paths; interpreting AST or IR directly is not part of the design.

## Backend Model

Current backend split:

- native: AST -> HIR -> MIR -> LIR -> CodegenPlan -> MachineProgram -> x86_64 assembly -> linked executable
- bytecode: AST -> HIR -> MIR -> BytecodeProgram `portable-v1`

The backend direction is described in [backend_strategy.md](/home/codewriter3000/Coding/calynda-lang/backend_strategy.md), and the current bytecode shape is described in [bytecode_isa.md](/home/codewriter3000/Coding/calynda-lang/bytecode_isa.md).

## Development

Run the full test suite:

```sh
./run_tests.sh
```

Useful build targets:

```sh
make calynda
make build_native
make emit_asm
make emit_bytecode
make test
make clean
```

The local VS Code syntax extension lives under [vscode-calynda/README.md](/home/codewriter3000/Coding/calynda-lang/vscode-calynda/README.md).

## Repository Layout

- `src/`: compiler, runtime, backend, and CLI sources
- `tests/`: regression tests for front-end, semantic, IR, backend, runtime, and CLI behavior
- `build/`: local build output
- `vscode-calynda/`: syntax highlighting and language configuration for `.cal` files

## Current Limits

- Native compilation currently targets Linux x86_64 SysV ELF.
- Final native linking currently uses `gcc`.
- The runtime surface is deliberately small and only covers the helpers the compiler lowers to today.
- The bytecode backend emits a portable compiled form, but this repository does not yet ship a bytecode runner.

## License

This project is licensed under the MIT License. See [LICENSE](/home/codewriter3000/Coding/calynda-lang/LICENSE).