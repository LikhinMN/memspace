# Memspace

Memspace is a blazing-fast, lightweight **Repository Intelligence Engine** built in C. It parses your codebase using [Tree-sitter](https://tree-sitter.github.io/tree-sitter/) to extract symbols and structural relationships, and exposes them to AI coding agents via a Model Context Protocol (MCP) server.

If you are tired of AI assistants hallucinating function names or aimlessly searching your codebase, Memspace acts as a perfectly accurate, sub-millisecond local graph brain for your AI.

## Features
- 🚀 **Extremely Fast**: Written entirely in C with an optimized SQLite backend.
- 🌳 **True Syntax Understanding**: Parses C, Python, and JavaScript ASTs natively via Tree-sitter.
- 🔍 **Relationship Mapping**: Not just definitions! Automatically tracks `calls` across functions to determine impact radius.
- 🤖 **Native MCP Server**: Fully compliant with the JSON-RPC 2.0 Model Context Protocol to plug directly into Claude, Cursor, and other AI agents.

## Installation

### Via NPM (Recommended)
Pre-compiled binaries are available for Windows, macOS, and Linux across x64 and ARM64 architectures.
```bash
npm install -g memspace
```

### Building from Source
If you prefer to compile the C source yourself, ensure you have `gcc` and `make` installed:
```bash
git clone https://github.com/your-username/memspace.git
cd memspace
make clean && make
```
The binary will be compiled to `bin/memspace`.

## Quick Start

1. **Initialize the workspace** in the root of your target repository:
```bash
memspace init
```

2. **Index your codebase**:
```bash
memspace index
```

3. **Keep it updated**: As you write new code, incrementally update the index instead of rebuilding it from scratch:
```bash
memspace update
```

## Connecting to your AI Assistant (MCP)

To give your AI assistant access to Memspace's intelligence, you need to register it as an MCP server (often called a "Skill" or "Tool"). Because we natively support JSON-RPC 2.0, it plugs directly into major AI clients.

### For Claude Desktop
Add the following to your `claude_desktop_config.json` file:
```json
{
  "mcpServers": {
    "memspace": {
      "command": "npx",
      "args": ["-y", "memspace", "serve"]
    }
  }
}
```

### For Cursor IDE
1. Open **Cursor Settings** -> **Features** -> **MCP**.
2. Click **+ Add new MCP server**.
3. Set **Type** to `command`.
4. Set **Name** to `memspace`.
5. Set **Command** to `npx -y memspace serve`.
6. Click **Save**.

### For Roo Code (VS Code Extension)
Click the **MCP Server** icon in the Roo Code chat panel and add:
```json
{
  "mcpServers": {
    "memspace": {
      "command": "npx",
      "args": ["-y", "memspace", "serve"]
    }
  }
}
```

Once configured, your AI will have access to powerful tools like:
- `get_symbol`: Fetches exact file, line, and signature of a symbol.
- `get_callers`: Finds every place a function is called.
- `impact_of`: Analyzes the direct and transitive blast radius of changing a specific function.
- `find_feature`: Performs a fuzzy text search across all symbols.

## Development

See [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to build, test, and contribute to Memspace.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
