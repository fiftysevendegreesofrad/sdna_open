rem test python2 and python3 in turn

set pythonexe=%python2exe%
set outputsuffix=py2

cmd /C run_debug_test.bat

set pythonexe=%python3exe%
set outputsuffix=py3

cmd /C run_debug_test.bat

rem Python 3 test only for the following:
%pythonexe% -u debug_test.py >testout_%outputsuffix%.txt 2>&1
cmd /C colourdiff correctout.txt testout_%outputsuffix%.txt 
%pythonexe% -u 3d_test.py >testout3d_%outputsuffix%.txt 2>&1
cmd /C colourdiff correctout3d.txt testout3d_%outputsuffix%.txt 
%pythonexe% -u partial_test.py >testout_partial_%outputsuffix%.txt 2>&1
cmd /C colourdiff correctout_partial.txt testout_partial_%outputsuffix%.txt 
%pythonexe% -u test_parallel_results.py >NUL
%pythonexe% -u prepare_barns_test.py >testout_prepbarns_%outputsuffix%.txt 2>&1
cmd /C colourdiff correctout_prepbarns.txt testout_prepbarns_%outputsuffix%.txt 

pause