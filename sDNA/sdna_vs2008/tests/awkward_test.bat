rem booleans in order: cont, overwrite, weight

del /S /Q awkward_test.gdb
xcopy /i /v awkward_blank.gdb awkward_test.gdb
c:\Python26\ArcGIS10.0\python.exe -u d:\sDNA\sDNA\sdna_vs2008\tests\awkward_test.py "awkward_test.gdb/awkward.shp" "" "" "ANGULAR" "" "n" "" "" ""
pause