# Backend Strategy

## Decision

Calynda for Wii now has two compiler backends and still rejects any interpreter path:

- Primary backend: PowerPC (Broadway) DOL executables for the Nintendo Wii
- Secondary backend: portable-v1 bytecode emitted directly from MIR
- Rejected path: interpreting AST, HIR, MIR, or any later IR directly

## Why This Split

The repository already has a real lowering path through HIR, MIR, LIR, codegen planning, machine emission, and now assembly emission. Throwing that away in favor of a VM-first path would waste the backend work that already exists.

At the same time, the language benefits from a portable execution target. The right way to add that is a bytecode compiler, not an interpreter. That preserves the requirement that execution remains compiled, while giving the project a second backend for portability and faster bring-up on non-Wii targets.

## Backend Boundaries

- Native backend path: AST -> HIR -> MIR -> LIR -> CodegenPlan -> MachineProgram -> PowerPC assembly -> DOL executable
- Bytecode backend path: AST -> HIR -> MIR -> BytecodeProgram portable-v1

MIR is the split point for the bytecode backend because it already has explicit control flow, closures, globals, calls, and throw.

## Runtime Direction

The runtime contract defined today is shared backend infrastructure, not an interpreter surface.

- PowerPC assembly now calls the runtime helpers directly and emits startup wrappers that make DOL executables runnable on Wii hardware or the Dolphin emulator.
- The bytecode compiler targets the same language-level operations and shares the same semantic boundary around runtime-backed features.
- If bytecode later needs backend-specific helper shims, those should sit on top of the same core runtime objects instead of duplicating language semantics.

## Near-Term Implications

- Keep improving the PowerPC (Broadway) backend now that it can assemble, link, and produce DOL executables for the Wii.
- Treat the current runtime ABI and object model as the canonical semantic boundary for dynamic features.
- Evolve the portable-v1 bytecode ISA and its MIR lowering without introducing an interpreter for an existing IR.
- Consider the Wii's hardware constraints (Broadway 729 MHz, 88 MB RAM) when evolving the runtime and backend.