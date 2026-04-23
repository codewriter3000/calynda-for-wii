# Calynda-for-Wii MCP Server

A [Model Context Protocol](https://modelcontextprotocol.io/) server that
turns this repository into a queryable knowledge base for AI assistants
and editor agents. It exposes compiler/decompiler documentation, the
Calynda language grammar, Wii hardware notes, and grep-like source
search — all over stdio with **zero third-party dependencies** (pure
Python ≥ 3.8).

## What it knows about

- The Calynda language (V2 canonical grammar + Wii fork EBNF)
- Compiler pipeline (tokenizer → parser → HIR → type checker → C emitter → devkitPPC)
- Decompiler internals (PowerPC decoder, CFG/SSA/types/classes passes, supported mnemonics)
- Wii hardware (Broadway CPU, GX, DSP, WPAD, Mii, MotionPlus, homebrew ABI)
- Game engine libraries (`calynda_gfx3d`, `calynda_math`, `calynda_physics`, `calynda_motion`)
- Repository layout and top-level docs (README, SETUP_GUIDE, RELEASE_NOTES, etc.)

## Tools

| Tool                      | Purpose                                                       |
| ------------------------- | ------------------------------------------------------------- |
| `list_knowledge`          | Catalog every indexed document.                               |
| `read_knowledge_file`     | Fetch a specific doc by id or repo-relative path.             |
| `search_knowledge`        | Ranked keyword search across all indexed docs.                |
| `get_calynda_grammar`     | Return the full EBNF (`variant: v2` or `fork`).               |
| `get_compiler_pipeline`   | Return the compiler-pipeline walkthrough doc.                 |
| `get_wii_internals`       | Return the Wii internals agent brief.                         |
| `get_ppc_mnemonics`       | List every PPC opcode the decompiler understands.             |
| `search_source`           | Regex search across the source tree.                          |
| `repo_tree`               | Top-level directory listing.                                  |

Every indexed document is also exposed as an MCP **resource** (`file://` URI).

## Running it

```bash
python3 /workspaces/calynda-for-wii/mcp-server/server.py
```

The server speaks MCP JSON-RPC 2.0 on stdin/stdout. Run the bundled
smoke test to verify it works in your environment:

```bash
python3 /workspaces/calynda-for-wii/mcp-server/test_server.py
```

## Wiring it into a client

### VS Code (GitHub Copilot / Claude)

Add this to your user `settings.json` (or a workspace `.vscode/mcp.json`):

```jsonc
{
  "mcp": {
    "servers": {
      "calynda-for-wii": {
        "command": "python3",
        "args": ["/workspaces/calynda-for-wii/mcp-server/server.py"]
      }
    }
  }
}
```

### Claude Desktop

Append to `~/.config/Claude/claude_desktop_config.json`
(or `%APPDATA%\Claude\claude_desktop_config.json` on Windows):

```jsonc
{
  "mcpServers": {
    "calynda-for-wii": {
      "command": "python3",
      "args": ["/workspaces/calynda-for-wii/mcp-server/server.py"]
    }
  }
}
```

### Any MCP-compatible client

The server binary is simply `python3 server.py` — point your client's
`stdio` transport at it.

## Adding new knowledge

1. Drop a new `.md` / `.ebnf` / `.h` / whatever file somewhere in the repo.
2. Add a `Doc(...)` entry to the `DOCS` list near the top of
   [server.py](./server.py).
3. Restart the client's MCP connection. That's the entire pipeline.

All paths are confined to the repo root — the server refuses to read
files outside `/workspaces/calynda-for-wii/`.
