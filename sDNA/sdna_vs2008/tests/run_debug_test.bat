rem "if this hangs, try pressing return, it might have been waiting for you to attach a debugger..."

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
