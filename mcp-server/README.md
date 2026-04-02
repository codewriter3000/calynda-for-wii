# Calynda MCP Server

An MCP (Model Context Protocol) server for the [Calynda](https://github.com/calynda-lang/calynda-lang) programming language.

## Features

- **Tools**: validate, analyze, explain, complete, examples, format
- **Resources**: grammar (EBNF), types, keywords, examples
- **Prompts**: explain-calynda, write-calynda, review-calynda

## Usage

```json
{
  "mcpServers": {
    "calynda": {
      "command": "node",
      "args": ["/path/to/mcp-server/dist/index.js"]
    }
  }
}
```

## Development

```sh
npm install
npm run build
node dist/index.js
```
