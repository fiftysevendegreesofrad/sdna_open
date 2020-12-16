@echo off
diff -q -w %1 %2
if errorlevel 1 (echo [101mTEST FAILED[0m) else (echo [92mPass[0m)