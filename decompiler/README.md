# Calynda Wii Decompiler

A static decompiler for Wii DOL executables. Produces a structured
intermediate representation (IR) plus annotated disassembly, with a
long-term goal of supporting recompilation-oriented workflows.

## v1 Scope

**In scope**
- DOL binary ingestion (single-file executables).
- PowerPC/Broadway static disassembly of `.text` sections.
- Function discovery and control-flow graph (CFG) recovery.
- Machine-independent decompiler IR with provenance (each IR node
  carries the source address and instruction id it was lifted from).
- Deterministic IR / JSON export and human-readable annotated
  disassembly listing.

**Explicitly out of scope for v1**
- REL modules and dynamic linker behavior.
- Guaranteed source-equivalent reconstruction.
- Full debug symbol recovery.
- Recompilation and binary-matching (tracked as post-v1, Phase 10).

## Pipeline

```
.dol  ->  loader  ->  disassembler  ->  cfg  ->  lifter  ->  ir  ->  emitters
                                                              |
                                               type / symbol hints
```

Each stage is a separate translation unit with a narrow header so the
pieces can be tested in isolation, mirroring the `compiler/` layout.

## Layout

```
decompiler/
  include/        public headers (one per stage)
  src/            implementations
  tests/          unit tests with synthetic fixtures
  Makefile        host-only build (GCC, C11)
```

## Building

```
make -C decompiler        # builds + runs all tests
make -C decompiler test   # runs tests
make -C decompiler clean
```

## Installing

```
sh decompiler/install.sh
sh decompiler/install.sh --prefix /usr/local
```

That installs `decompile` into your chosen `bin` directory so it can be
run directly from the shell.

## Design invariants

1. All DOL fields are 32-bit big-endian; the loader is the only code
   that touches raw bytes. Downstream stages consume typed views.
2. Every IR node must carry provenance (source DOL offset or virtual
   address) so output is auditable and diffable.
3. Emitters must be deterministic: stable IDs, stable ordering, no
   hash-order dependence. This is required for golden tests.
4. No stage mutates input buffers; the loader owns the DOL image and
   hands out `const`-qualified views.
