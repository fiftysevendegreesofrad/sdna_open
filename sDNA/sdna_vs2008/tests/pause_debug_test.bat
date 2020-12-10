rem test python2 and python3 in turn

set pythonexe=%python2exe%
set outputsuffix=py2

cmd /C run_debug_test.bat

set pythonexe=%python3exe%
set outputsuffix=py3

cmd /C run_debug_test.bat

pause