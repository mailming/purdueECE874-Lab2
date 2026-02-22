# Lab 2 impl-3: Avalon Tax Engine (C++)

Same specification as impl-1 (Python) and impl-2 (Go). CLI: `--inputFile`/`--outputFile` or `--input`/`--output`.

## Dependencies

- **nlohmann/json** — single header at `nlohmann/json.hpp` (included in repo).
- **C++17** compiler: g++, clang++, or MSVC.

## Build

### Linux / macOS / MinGW (g++)

```bash
cd impl-3
g++ -std=c++17 -O2 -o tax_engine main.cpp
```

### Windows (Visual Studio)

Open "x64 Native Tools Command Prompt for VS" or run from Developer PowerShell:

```cmd
cl /EHsc /std:c++17 /O2 /Fe:tax_engine.exe main.cpp
```

## Run

```bash
./tax_engine --input ../lab2-smoke-test-cases.ndjson --output ../impl-3-smoke-out.ndjson
```

Or use the version runner from repo root (after building):

```bash
python lab2-version-runner.py --version1 "impl-3\run_tax_engine.bat" --inputFile lab2-smoke-test-cases.ndjson --outputFilePrefix out
```
