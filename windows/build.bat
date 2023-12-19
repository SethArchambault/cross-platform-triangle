@echo off
cl /Debug /std:c++20 /nologo /Zi main.cpp
if %errorlevel% neq 0 exit /b %errorlevel%
main.exe
