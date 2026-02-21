# Lab 2 impl-2: Avalon Tax Engine (Go)

Same specification as impl-1 (Python). CLI: `--inputFile`/`--outputFile` or `--input`/`--output`.

## Build and run (requires [Go](https://go.dev/dl/) installed)

```bash
cd impl-2
go build -o tax_engine.exe .
./tax_engine.exe --input ../lab2-smoke-test-cases.ndjson --output ../impl-2-smoke-out.ndjson
```

Or use the runner from repo root:

```bash
python lab2-version-runner.py --version1 "impl-2\run_tax_engine.bat" --inputFile lab2-smoke-test-cases.ndjson --outputFilePrefix out
```

(On Windows use `impl-2\run_tax_engine.bat`; build first with `go build -o tax_engine.exe .` inside impl-2.)
