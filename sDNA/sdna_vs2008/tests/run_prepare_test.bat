%pythonexe% -u ..\..\..\arcscripts\bin\sdnaprepare.py -i prep_crash_shapefile\shapefilepreparecrash -o prep_crash_shapefile\out_%outputsuffix% --dll %sdnadll%  "action=repair;splitlinks;isolated;nearmisses;duplicates;xytol=0.01" >testout_prep_%outputsuffix%.txt 

%pythonexe% -u prepare_test_new.py >>testout_prep_%outputsuffix%.txt 2>&1

cmd /C mydiff correctout_prep.txt testout_prep_%outputsuffix%.txt 