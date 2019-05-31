%pythonexe% -u debug_test.py >quicktestout.txt 2>&1
diff -q -w correctout.txt quicktestout.txt 
pause