# Contributing to Memspace

We welcome contributions to Memspace! Here's how you can help.

## Project Structure
- `cli/`: Command-line interface logic (`init`, `index`, `update`, `serve`).
- `core/`: Core graph engine, SQLite indexing, and Tree-sitter parsing logic.
- `vendor/`: Third-party dependencies (cJSON, SQLite amalgamation, Tree-sitter core and grammars).
- `npm/`: Node.js wrapper scripts and multi-architecture package distribution structure.
- `tests/`: Basic test suites.

## Building Locally
Ensure you have `gcc` and `make` installed.

```bash
make clean
make
```
This will compile a static binary to `bin/memspace`.

## Running Tests
To ensure you haven't broken existing functionality:
```bash
make test
```

## Making Changes
1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/my-new-feature`).
3. Write your C code. Ensure you check for memory leaks and always validate `malloc` returns.
4. If modifying the database schema in `core/index.c`, please make sure the SQLite `CREATE TABLE IF NOT EXISTS` commands are updated cleanly.
5. Run tests via `make test`.
6. Submit a Pull Request!

## Code Style
- Stick to standard C11.
- Avoid memory leaks: ensure every `malloc`, `calloc`, and `strdup` has a corresponding `free`.
- Always check the return values of `sqlite3_prepare_v2` and ensure `sqlite3_finalize` is called.
