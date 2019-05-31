rem booleans in order: cont, overwrite, weight

del /S /Q test.gdb
xcopy /i /v blank_test.gdb test.gdb
c:\Python26\ArcGIS10.0\python.exe -u d:\sDNA\sDNA\sdna_vs2008\tests\gdb_test.py "test.gdb/roadlink_blank" "DirectedNode_NegativeOrientation_GradeSeparation" "DirectedNode_PositiveOrientation_GradeSeparation" "ANGULAR" "" "400,n" "True" "True" ""