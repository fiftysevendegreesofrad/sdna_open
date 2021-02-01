rem "if this hangs, try pressing return, it might have been waiting for you to attach a debugger..."

del testout_learn_%outputsuffix%.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnalearn.py --calibfile tiny.shp --target MADn --vars MGLAn,MCFn --boxcoxtarget --bcregex ".*" --mode single_best_variable --output regtestout_single_%outputsuffix%.txt --resids regtestresids_%outputsuffix%.shp >>testout_learn_%outputsuffix%.txt 2>>&1
%pythonexe% -u ..\..\..\arcscripts\bin\sdnalearn.py --calibfile tiny.shp --target MADn --vars MGLAn,MCFn --mode multiple_variables --output regtestout_multiple_%outputsuffix%.txt >>testout_learn_%outputsuffix%.txt 2>>&1
%pythonexe% -u ..\..\..\arcscripts\bin\sdnapredict.py --input tiny --output tinypred_%outputsuffix%.shp --modelfile regtestout_multiple_%outputsuffix%.txt --predvarname prediction >>testout_learn_%outputsuffix%.txt 2>>&1

rem remove all numbers after :,= as they vary with random seed. unlike the rest of the suite, this particular test is to check i/o works not numeric backend.
sed 's/[,:=].*[0-9].*//' testout_learn_%outputsuffix%.txt >testout_learn_%outputsuffix%_fordiff.txt

cmd /C mydiff correctout_learn.txt testout_learn_%outputsuffix%_fordiff.txt

pause

EXIT /B 0

@echo off
echo running table tests on %sdnadll%
del testout_table_%outputsuffix%.txt
del tabletestout_%outputsuffix%.*
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=zonetest;tables=zonetesttable.csv,zonetesttable2.csv" --om "net=tabletestout_%outputsuffix%.shp" "linkonly;zonesums=lulinks=landuse*FULLlf@origzone,eucsum=euc@destzone;origweightformula=zoneweight*one*dztest*landuse*FULLlf/lulinks;destweightformula=eucsum;" >testout_table_%outputsuffix%.txt 
%pythonexe% ..\..\..\arcscripts\shp2txt.py tabletestout_%outputsuffix% >>testout_table_%outputsuffix%.txt 
echo "duplicate zone table/sum test" >>testout_table_%outputsuffix%.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=zonetest;tables=zonetesttable.csv" --om "net=tabletestout1_%outputsuffix%.shp" "linkonly;zonesums=one=1@destzone" >>testout_table_%outputsuffix%.txt 
echo "duplicate zone and net data test" >>testout_table_%outputsuffix%.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=zonetest;tables=zonetesttableduplicate.csv" --om "net=tabletestout2_%outputsuffix%.shp" "linkonly" >>testout_table_%outputsuffix%.txt 
@echo on
cmd /C mydiff correctout_table.txt testout_table_%outputsuffix%.txt
echo
@echo off
echo running OD tests on %sdnadll%
del odtestout_%outputsuffix%* odtestout_stdout_%outputsuffix%.txt
del odtest_%outputsuffix%.shp
%pythonexe% -u make_od_test_shp.py >NUL 2>NUL
del testout_od_%outputsuffix%* testout_od_skim_%outputsuffix%*
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=skimtest" --om "skim=testout_od_skim_%outputsuffix%.csv" "radii=n;outputskim;skimzone=zone;nonetdata;metric=euclidean" >>odtestout_stdout_%outputsuffix%.txt 2>>&1
cat testout_od_skim_%outputsuffix%.csv >>testout_od_%outputsuffix%.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=skimtestzeroweight" --om "skim=testout_od_skimzeroweight_%outputsuffix%.csv" "radii=n;outputskim;skimzone=zone;nonetdata;metric=euclidean;weight=weight" >>odtestout_stdout_%outputsuffix%.txt 2>>NUL
cat testout_od_skimzeroweight_%outputsuffix%.csv >>testout_od_%outputsuffix%.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=odtest;tables=testodmatrix.csv" --om "net=odtestoutsparse_%outputsuffix%" "radii=n;odmatrix" >>odtestout_stdout_%outputsuffix%.txt 2>>&1
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=odtest;tables=testodmatrix-nonsparse.csv" --om "net=odtestoutnonsparse_%outputsuffix%" "radii=n;odmatrix" >>odtestout_stdout_%outputsuffix%.txt 2>>&1
%pythonexe% ..\..\..\arcscripts\shp2txt.py odtestoutsparse_%outputsuffix% odtestoutnonsparse_%outputsuffix% >>testout_od_%outputsuffix%.txt 2>&1
@echo on
cmd /C mydiff correctout_od.txt testout_od_%outputsuffix%.txt

cmd /C run_geom_test.bat
cmd /C run_prepare_test.bat
