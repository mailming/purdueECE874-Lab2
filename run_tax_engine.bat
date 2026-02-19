@echo off
REM Shim so lab2-version-runner.py can invoke the tax engine (runner uses --input / --output).
python "%~dp0tax_engine.py" %*
