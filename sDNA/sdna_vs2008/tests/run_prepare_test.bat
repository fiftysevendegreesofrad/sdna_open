%pythonexe% -u ..\..\..\arcscripts\bin\sdnaprepare.py -i prep_crash_shapefile\shapefilepreparecrash -o prep_crash_shapefile\out --dll %sdnadll%  "action=repair;splitlinks;isolated;nearmisses;duplicates;xytol=0.01" >testout_prep.txt 

%pythonexe% -u prepare_test_new.py >>testout_prep.txt 2>&1

diff -q -w correctout_prep.txt testout_prep.txt 