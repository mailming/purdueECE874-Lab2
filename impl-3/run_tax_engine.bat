@echo off
REM Run C++ tax engine. Build first: g++ -std=c++17 -O2 -o tax_engine.exe main.cpp
cd /d "%~dp0"
if exist tax_engine.exe (
  tax_engine.exe %*
) else (
  echo Build impl-3 first: g++ -std=c++17 -O2 -o tax_engine.exe main.cpp
  exit /b 1
)
