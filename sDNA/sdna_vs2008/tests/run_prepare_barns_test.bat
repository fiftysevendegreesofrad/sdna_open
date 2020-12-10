%pythonexe% -u prepare_barns_test.py >testout_prepbarns_%outputsuffix%.txt 2>&1
diff -q -w correctout_prepbarns.txt testout_prepbarns_%outputsuffix%.txt 