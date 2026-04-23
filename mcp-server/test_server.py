#!/usr/bin/env python3
"""Smoke test for the Calynda MCP server. Spawns it over stdio and
exercises initialize + tools/list + a handful of tools/call requests."""
import json
import os
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SERVER = HERE / "server.py"

def rpc(proc, req):
    proc.stdin.write(json.dumps(req) + "\n")
    proc.stdin.flush()
    return json.loads(proc.stdout.readline())

def notify(proc, method, params=None):
    msg = {"jsonrpc": "2.0", "method": method}
    if params is not None:
        msg["params"] = params
    proc.stdin.write(json.dumps(msg) + "\n")
    proc.stdin.flush()

def main() -> int:
    proc = subprocess.Popen(
        [sys.executable, str(SERVER)],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=sys.stderr,
        text=True, bufsize=1,
    )
    assert proc.stdin and proc.stdout

    r = rpc(proc, {"jsonrpc": "2.0", "id": 1, "method": "initialize",
                   "params": {"protocolVersion": "2024-11-05",
                              "capabilities": {},
                              "clientInfo": {"name": "smoke", "version": "0"}}})
    assert r["result"]["serverInfo"]["name"] == "calynda-for-wii-kb", r
    notify(proc, "notifications/initialized")

    r = rpc(proc, {"jsonrpc": "2.0", "id": 2, "method": "tools/list"})
    names = [t["name"] for t in r["result"]["tools"]]
    expected = {"list_knowledge", "read_knowledge_file", "search_knowledge",
                "get_calynda_grammar", "get_compiler_pipeline",
                "get_wii_internals", "get_ppc_mnemonics", "search_source",
                "repo_tree"}
    assert expected <= set(names), names

    for name, args in [
        ("list_knowledge",      {}),
        ("repo_tree",           {}),
        ("search_knowledge",    {"query": "paired single", "limit": 3}),
        ("get_ppc_mnemonics",   {}),
        ("read_knowledge_file", {"id": "pipeline"}),
        ("get_calynda_grammar", {"variant": "fork"}),
        ("search_source",       {"query": "ppc_decode\\b",
                                 "glob": "decompiler/**/*.c", "limit": 5}),
    ]:
        r = rpc(proc, {"jsonrpc": "2.0", "id": 10, "method": "tools/call",
                       "params": {"name": name, "arguments": args}})
        res = r["result"]
        assert not res.get("isError"), (name, res)
        text = res["content"][0]["text"]
        assert len(text) > 0, name
        print(f"ok  {name:<22s}  {len(text):6d} chars")

    r = rpc(proc, {"jsonrpc": "2.0", "id": 20, "method": "resources/list"})
    assert len(r["result"]["resources"]) > 0
    print(f"ok  resources/list         {len(r['result']['resources'])} resources")

    # Invalid tool name -> isError.
    r = rpc(proc, {"jsonrpc": "2.0", "id": 21, "method": "tools/call",
                   "params": {"name": "nope", "arguments": {}}})
    assert r["result"]["isError"] is True
    print("ok  invalid tool error path")

    proc.stdin.close()
    proc.wait(timeout=5)
    print("all smoke tests passed")
    return 0

if __name__ == "__main__":
    sys.exit(main())
