%pythonexe% -u prepare_barns_test.py >testout_prepbarns.txt 2>&1
diff -q -w correctout_prepbarns.txt testout_prepbarns.txt 