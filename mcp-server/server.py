#!/usr/bin/env python3
"""
Calynda-for-Wii MCP server.

A Model Context Protocol server that exposes the repository as a
knowledge base: documentation, language grammars, agent briefs, and
source-level search. Runs over stdio with zero third-party
dependencies (pure Python 3.8+).

Tools provided:
    list_knowledge          — catalog of indexed documents
    read_knowledge_file     — fetch a document by id or path
    search_knowledge        — keyword search across all docs
    get_calynda_grammar     — full EBNF (v2 or Wii fork)
    get_compiler_pipeline   — compiler stages overview
    get_wii_internals       — Wii hardware / homebrew notes
    get_ppc_mnemonics       — list of PPC opcodes handled by the decompiler
    search_source           — grep-like search across the source tree
    repo_tree               — high-level directory listing

Resources provided:
    Every indexed document is also exposed as a resource (file:// URI).

Protocol: JSON-RPC 2.0 (one message per line) over stdin/stdout, per
the MCP 2024-11-05 specification.
"""
from __future__ import annotations

import json
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Dict, Iterable, List, Optional, Tuple

# --------------------------------------------------------------------
# Repository layout

REPO_ROOT = Path(__file__).resolve().parent.parent
MAX_READ_BYTES = 256 * 1024       # 256 KiB hard cap when reading
MAX_SEARCH_HITS = 50
MAX_SNIPPET_CHARS = 240

# Allow-listed knowledge documents. Paths are relative to REPO_ROOT.
# Adding a new entry here is the only requirement to ship new knowledge.
@dataclass(frozen=True)
class Doc:
    id: str
    path: str
    title: str
    summary: str

DOCS: List[Doc] = [
    Doc("readme",            "README.md",
        "Project README",
        "Top-level overview of Calynda for Wii."),
    Doc("setup_guide",       "SETUP_GUIDE.md",
        "Setup Guide",
        "How to bring up a dev environment for building Calynda."),
    Doc("contributing",      "CONTRIBUTING.md",
        "Contributing Guide",
        "Contribution rules, style, and workflow."),
    Doc("release_notes",     "RELEASE_NOTES.md",
        "Release Notes",
        "Version-by-version change log."),
    Doc("version_wishlist",  "version_wishlist.md",
        "Version Wishlist",
        "Planned features organized by future release."),
    Doc("backend_strategy",  "backend_strategy.md",
        "Backend Strategy",
        "Compiler backend design rationale (C emitter, native)."),
    Doc("bytecode_isa",      "bytecode_isa.md",
        "Bytecode ISA",
        "Calynda VM bytecode instruction set."),
    Doc("pipeline",          "compiler/COMPILATION_PIPELINE.md",
        "Compilation Pipeline",
        "Detailed walkthrough of every compiler stage."),
    Doc("grammar_v2",        "compiler/calynda.ebnf",
        "Calynda V2 Grammar (canonical EBNF)",
        "The canonical Calynda language grammar."),
    Doc("grammar_fork",      "compiler/calynda_fork.ebnf",
        "Calynda Wii Fork Grammar (EBNF)",
        "Wii-targeted grammar fork: removes 64-bit types, adds boot/extern c."),
    Doc("decompiler_readme", "decompiler/README.md",
        "Decompiler README",
        "Overview of the PowerPC / DOL / Wii ISO decompiler."),
    Doc("agent_calynda",     ".github/agents/calynda.agent.md",
        "Agent Brief: Calynda Compiler",
        "Comprehensive compiler/runtime/tooling reference."),
    Doc("agent_game_engine", ".github/agents/game-engine.agent.md",
        "Agent Brief: Game Engine Libraries",
        "calynda_gfx3d/math/physics/motion reference."),
    Doc("agent_wii",         ".github/agents/wii-internals.agent.md",
        "Agent Brief: Wii Internals",
        "Broadway CPU, GX, DSP, WPAD, Mii, and other Wii subsystems."),
    Doc("vscode_ext",        "vscode-calynda/README.md",
        "VS Code Extension README",
        "Syntax highlighting extension for .cal files."),
    Doc("libwiigui",         "libs/libwiigui/README.md",
        "libwiigui README",
        "GUI library used for on-screen UI in Wii builds."),
    Doc("ppc_header",        "decompiler/include/ppc.h",
        "Decompiler: PpcOp enum",
        "Public header enumerating supported PowerPC opcodes."),
]

DOC_BY_ID = {d.id: d for d in DOCS}


# --------------------------------------------------------------------
# Helpers

def _abs(doc_path: str) -> Path:
    p = (REPO_ROOT / doc_path).resolve()
    # Confine to the repo root to avoid path traversal.
    if REPO_ROOT not in p.parents and p != REPO_ROOT:
        raise ValueError(f"path escapes repo root: {doc_path}")
    return p


def _safe_read(path: Path, max_bytes: int = MAX_READ_BYTES) -> str:
    data = path.read_bytes()[:max_bytes + 1]
    truncated = len(data) > max_bytes
    text = data[:max_bytes].decode("utf-8", errors="replace")
    if truncated:
        text += f"\n\n... [truncated at {max_bytes} bytes]\n"
    return text


def _tokenize(q: str) -> List[str]:
    return [t for t in re.findall(r"[A-Za-z0-9_]+", q.lower()) if t]


def _score_line(line_lower: str, tokens: List[str]) -> int:
    score = 0
    for t in tokens:
        if t in line_lower:
            score += 1
    return score


# --------------------------------------------------------------------
# Tool implementations

def tool_list_knowledge(_args: Dict[str, Any]) -> str:
    rows = [f"{d.id:22s}  {d.title}\n    {d.path}\n    {d.summary}"
            for d in DOCS]
    return "Indexed documents (id / title / path / summary):\n\n" + \
           "\n\n".join(rows)


def tool_read_knowledge_file(args: Dict[str, Any]) -> str:
    ident = args.get("id") or args.get("path")
    if not ident:
        raise ValueError("provide 'id' or 'path'")
    if ident in DOC_BY_ID:
        doc = DOC_BY_ID[ident]
        p = _abs(doc.path)
        header = f"# {doc.title}\nsource: {doc.path}\n\n"
    else:
        # Fall back: treat as a repo-relative path, but only allow
        # files that match one of the indexed docs' paths.
        match = next((d for d in DOCS if d.path == ident), None)
        if not match:
            raise ValueError(f"unknown doc id or path: {ident!r}")
        doc = match
        p = _abs(doc.path)
        header = f"# {doc.title}\nsource: {doc.path}\n\n"
    return header + _safe_read(p)


def tool_search_knowledge(args: Dict[str, Any]) -> str:
    query = args.get("query", "")
    limit = int(args.get("limit", 10))
    tokens = _tokenize(query)
    if not tokens:
        raise ValueError("query must contain searchable characters")

    hits: List[Tuple[int, Doc, int, str]] = []  # (score, doc, lineno, line)
    for doc in DOCS:
        try:
            text = _abs(doc.path).read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for i, line in enumerate(text.splitlines(), start=1):
            s = _score_line(line.lower(), tokens)
            if s > 0:
                hits.append((s, doc, i, line.strip()[:MAX_SNIPPET_CHARS]))

    hits.sort(key=lambda h: (-h[0], h[1].id, h[2]))
    hits = hits[:limit]
    if not hits:
        return f"No matches for query: {query!r}"
    out = [f"{len(hits)} match(es) for {query!r}:"]
    for score, doc, lineno, line in hits:
        out.append(f"  [{score}] {doc.id}:{lineno}  {line}")
    return "\n".join(out)


def tool_get_calynda_grammar(args: Dict[str, Any]) -> str:
    variant = args.get("variant", "fork")
    if variant not in ("v2", "fork"):
        raise ValueError("variant must be 'v2' or 'fork'")
    doc = DOC_BY_ID["grammar_fork" if variant == "fork" else "grammar_v2"]
    return f"# {doc.title}\nsource: {doc.path}\n\n" + _safe_read(_abs(doc.path))


def tool_get_compiler_pipeline(_args: Dict[str, Any]) -> str:
    doc = DOC_BY_ID["pipeline"]
    return f"# {doc.title}\nsource: {doc.path}\n\n" + _safe_read(_abs(doc.path))


def tool_get_wii_internals(_args: Dict[str, Any]) -> str:
    doc = DOC_BY_ID["agent_wii"]
    return f"# {doc.title}\nsource: {doc.path}\n\n" + _safe_read(_abs(doc.path))


def tool_get_ppc_mnemonics(_args: Dict[str, Any]) -> str:
    """Extract supported mnemonics from the decompiler's ppc.h enum."""
    header = _abs("decompiler/include/ppc.h").read_text(errors="replace")
    ops: List[Tuple[str, str]] = []
    for line in header.splitlines():
        m = re.match(r"\s*PPC_OP_([A-Z0-9_]+)\s*[,=}]?(?:\s*/\*\s*(.+?)\s*\*/)?",
                     line)
        if not m:
            continue
        name = m.group(1)
        if name == "UNKNOWN":
            continue
        comment = (m.group(2) or "").strip()
        ops.append((name.lower(), comment))
    uniq: Dict[str, str] = {}
    for name, comment in ops:
        # Keep the richer comment if we see the mnemonic twice.
        if name not in uniq or (comment and not uniq[name]):
            uniq[name] = comment
    lines = [f"{len(uniq)} PowerPC mnemonics decoded by the decompiler:\n"]
    for name in sorted(uniq):
        c = uniq[name]
        lines.append(f"  {name:<10s} {c}" if c else f"  {name}")
    return "\n".join(lines)


def tool_search_source(args: Dict[str, Any]) -> str:
    pattern = args.get("query", "")
    glob = args.get("glob", "**/*.c")
    limit = int(args.get("limit", 40))
    if not pattern:
        raise ValueError("query is required")
    if ".." in glob or glob.startswith("/"):
        raise ValueError("glob must be repo-relative")
    try:
        regex = re.compile(pattern)
    except re.error as e:
        raise ValueError(f"invalid regex: {e}")

    skip_dirs = {".git", "build", "node_modules", "decomp-workspace", "-p"}
    hits: List[str] = []
    for path in REPO_ROOT.glob(glob):
        if not path.is_file():
            continue
        if any(part in skip_dirs for part in path.relative_to(REPO_ROOT).parts):
            continue
        try:
            with path.open("r", encoding="utf-8", errors="replace") as f:
                for i, line in enumerate(f, start=1):
                    if regex.search(line):
                        rel = path.relative_to(REPO_ROOT)
                        snippet = line.rstrip()[:MAX_SNIPPET_CHARS]
                        hits.append(f"{rel}:{i}: {snippet}")
                        if len(hits) >= limit:
                            break
        except OSError:
            continue
        if len(hits) >= limit:
            break
    if not hits:
        return f"No matches for pattern {pattern!r} in {glob!r}"
    return f"{len(hits)} match(es) for {pattern!r} in {glob!r}:\n" + \
           "\n".join(hits)


def tool_repo_tree(_args: Dict[str, Any]) -> str:
    top = []
    for p in sorted(REPO_ROOT.iterdir()):
        if p.name.startswith(".") and p.name not in (".github",):
            continue
        top.append(f"{p.name}/" if p.is_dir() else p.name)
    return "Top-level layout of calynda-for-wii:\n  " + "\n  ".join(top)


# --------------------------------------------------------------------
# Tool registry

@dataclass
class ToolSpec:
    name: str
    description: str
    input_schema: Dict[str, Any]
    handler: Callable[[Dict[str, Any]], str]


TOOLS: List[ToolSpec] = [
    ToolSpec(
        name="list_knowledge",
        description="List every document in the knowledge index "
                    "(title, id, path, summary). Start here to see what "
                    "knowledge the server can serve.",
        input_schema={"type": "object", "properties": {}, "additionalProperties": False},
        handler=tool_list_knowledge,
    ),
    ToolSpec(
        name="read_knowledge_file",
        description="Read a specific indexed document, by id (e.g. "
                    "'pipeline', 'agent_wii') or by its repo-relative path.",
        input_schema={
            "type": "object",
            "properties": {
                "id":   {"type": "string", "description": "Document id from list_knowledge."},
                "path": {"type": "string", "description": "Repo-relative path (alternative to id)."},
            },
            "additionalProperties": False,
        },
        handler=tool_read_knowledge_file,
    ),
    ToolSpec(
        name="search_knowledge",
        description="Keyword search across all indexed documents. Returns "
                    "ranked doc:line snippets.",
        input_schema={
            "type": "object",
            "properties": {
                "query": {"type": "string"},
                "limit": {"type": "integer", "minimum": 1, "maximum": 50, "default": 10},
            },
            "required": ["query"],
            "additionalProperties": False,
        },
        handler=tool_search_knowledge,
    ),
    ToolSpec(
        name="get_calynda_grammar",
        description="Return the full EBNF grammar for the Calynda language.",
        input_schema={
            "type": "object",
            "properties": {
                "variant": {
                    "type": "string", "enum": ["v2", "fork"], "default": "fork",
                    "description": "'v2' for the canonical grammar, 'fork' for the Wii-targeted subset."
                }
            },
            "additionalProperties": False,
        },
        handler=tool_get_calynda_grammar,
    ),
    ToolSpec(
        name="get_compiler_pipeline",
        description="Return COMPILATION_PIPELINE.md — every compiler stage "
                    "from tokenizer through C emission to linking.",
        input_schema={"type": "object", "properties": {}, "additionalProperties": False},
        handler=tool_get_compiler_pipeline,
    ),
    ToolSpec(
        name="get_wii_internals",
        description="Return the Wii internals agent brief: Broadway CPU, "
                    "GX, DSP, WPAD, Mii, and homebrew specifics.",
        input_schema={"type": "object", "properties": {}, "additionalProperties": False},
        handler=tool_get_wii_internals,
    ),
    ToolSpec(
        name="get_ppc_mnemonics",
        description="List every PowerPC mnemonic decoded by the decompiler, "
                    "extracted from decompiler/include/ppc.h.",
        input_schema={"type": "object", "properties": {}, "additionalProperties": False},
        handler=tool_get_ppc_mnemonics,
    ),
    ToolSpec(
        name="search_source",
        description="Regex search across the source tree. Skips build/, "
                    ".git/, decomp-workspace/, and similar.",
        input_schema={
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "Python regex."},
                "glob":  {"type": "string", "default": "**/*.c",
                          "description": "Repo-relative glob (e.g. 'compiler/src/**/*.c')."},
                "limit": {"type": "integer", "minimum": 1, "maximum": 200, "default": 40},
            },
            "required": ["query"],
            "additionalProperties": False,
        },
        handler=tool_search_source,
    ),
    ToolSpec(
        name="repo_tree",
        description="Return the top-level directory layout of the "
                    "calynda-for-wii repository.",
        input_schema={"type": "object", "properties": {}, "additionalProperties": False},
        handler=tool_repo_tree,
    ),
]

TOOL_BY_NAME = {t.name: t for t in TOOLS}


# --------------------------------------------------------------------
# MCP JSON-RPC stdio server

PROTOCOL_VERSION = "2024-11-05"

def _resource_uri(doc: Doc) -> str:
    return f"file://{_abs(doc.path).as_posix()}"


def handle_initialize(_params: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "protocolVersion": PROTOCOL_VERSION,
        "capabilities": {
            "tools":     {"listChanged": False},
            "resources": {"listChanged": False, "subscribe": False},
        },
        "serverInfo": {
            "name":    "calynda-for-wii-kb",
            "version": "0.1.0",
        },
    }


def handle_tools_list(_params: Dict[str, Any]) -> Dict[str, Any]:
    return {"tools": [
        {"name": t.name, "description": t.description, "inputSchema": t.input_schema}
        for t in TOOLS
    ]}


def handle_tools_call(params: Dict[str, Any]) -> Dict[str, Any]:
    name = params.get("name")
    args = params.get("arguments") or {}
    tool = TOOL_BY_NAME.get(name)
    if not tool:
        return {
            "isError": True,
            "content": [{"type": "text", "text": f"unknown tool: {name!r}"}],
        }
    try:
        text = tool.handler(args)
    except Exception as e:  # noqa: BLE001 — surface any error to the client
        return {
            "isError": True,
            "content": [{"type": "text", "text": f"{type(e).__name__}: {e}"}],
        }
    return {"content": [{"type": "text", "text": text}]}


def handle_resources_list(_params: Dict[str, Any]) -> Dict[str, Any]:
    return {"resources": [
        {
            "uri":         _resource_uri(d),
            "name":        d.title,
            "description": d.summary,
            "mimeType":    "text/markdown" if d.path.endswith(".md") else
                           "text/plain",
        }
        for d in DOCS
    ]}


def handle_resources_read(params: Dict[str, Any]) -> Dict[str, Any]:
    uri = params.get("uri", "")
    match = next((d for d in DOCS if _resource_uri(d) == uri), None)
    if not match:
        raise ValueError(f"unknown resource uri: {uri!r}")
    return {"contents": [{
        "uri":      uri,
        "mimeType": "text/markdown" if match.path.endswith(".md") else
                    "text/plain",
        "text":     _safe_read(_abs(match.path)),
    }]}


METHODS: Dict[str, Callable[[Dict[str, Any]], Any]] = {
    "initialize":     handle_initialize,
    "tools/list":     handle_tools_list,
    "tools/call":     handle_tools_call,
    "resources/list": handle_resources_list,
    "resources/read": handle_resources_read,
    # Ping is optional but cheap.
    "ping":           lambda _p: {},
}


def _write(msg: Dict[str, Any]) -> None:
    sys.stdout.write(json.dumps(msg, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def _error(req_id: Any, code: int, message: str) -> None:
    _write({"jsonrpc": "2.0", "id": req_id,
            "error": {"code": code, "message": message}})


def serve() -> None:
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except json.JSONDecodeError as e:
            _error(None, -32700, f"parse error: {e}")
            continue

        method = msg.get("method")
        req_id = msg.get("id")
        params = msg.get("params") or {}

        # Notifications carry no id; we must not reply to them.
        if req_id is None:
            # e.g. "notifications/initialized" — nothing to do.
            continue

        handler = METHODS.get(method)
        if not handler:
            _error(req_id, -32601, f"method not found: {method}")
            continue
        try:
            result = handler(params)
        except Exception as e:  # noqa: BLE001
            _error(req_id, -32000, f"{type(e).__name__}: {e}")
            continue
        _write({"jsonrpc": "2.0", "id": req_id, "result": result})


if __name__ == "__main__":
    try:
        serve()
    except (BrokenPipeError, KeyboardInterrupt):
        pass
