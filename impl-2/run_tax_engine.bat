@echo off
REM Run Go tax engine. Requires Go installed (https://go.dev/dl/).
cd /d "%~dp0"
if exist tax_engine.exe (
  tax_engine.exe %*
) else (
  go run . %*
)
