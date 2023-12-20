@echo off
cl /Debug /std:c++20 /nologo /Zi win_main.cpp
if %errorlevel% neq 0 exit /b %errorlevel%
win_main.exe
