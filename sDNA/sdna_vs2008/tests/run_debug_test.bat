rem "if this hangs, try pressing return, it might have been waiting for you to attach a debugger..."

%pythonexe% -u debug_test.py >testout.txt 2>&1
diff -q -w correctout.txt testout.txt 

echo

@echo off
echo running table tests on %sdnadll%
del testout_table.txt
del tabletestout.*
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=zonetest;tables=zonetesttable.csv,zonetesttable2.csv" --om "net=tabletestout.shp" "linkonly;zonesums=lulinks=landuse*FULLlf@origzone,eucsum=euc@destzone;origweightformula=zoneweight*one*dztest*landuse*FULLlf/lulinks;destweightformula=eucsum;" >testout_table.txt 2>>&1
%pythonexe% ..\..\..\arcscripts\shp2txt.py tabletestout >>testout_table.txt 2>&1
echo "duplicate zone table/sum test" >>testout_table.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=zonetest;tables=zonetesttable.csv" --om "net=tabletestout1.shp" "linkonly;zonesums=one=1@destzone" >>testout_table.txt 2>>&1
echo "duplicate zone and net data test" >>testout_table.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=zonetest;tables=zonetesttableduplicate.csv" --om "net=tabletestout2.shp" "linkonly" >>testout_table.txt 2>>&1
@echo on
diff -q -w correctout_table.txt testout_table.txt

@echo off
echo running OD tests on %sdnadll%
del odtestout*
del odtest.shp
%pythonexe% -u make_od_test_shp.py >NUL 2>NUL
del testout_od.txt testout_od_skim*
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=skimtest" --om "skim=testout_od_skim.csv" "radii=n;outputskim;skimzone=zone;nonetdata;metric=euclidean" >NUL 2>>testout_od.txt
cat testout_od_skim.csv >>testout_od.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=skimtestzeroweight" --om "skim=testout_od_skimzeroweight.csv" "radii=n;outputskim;skimzone=zone;nonetdata;metric=euclidean;weight=weight" >NUL 2>>testout_od.txt
cat testout_od_skimzeroweight.csv >>testout_od.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=odtest;tables=testodmatrix.csv" --om "net=odtestoutsparse" "radii=n;odmatrix" >NUL 2>>testout_od.txt
%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py --dll %sdnadll% --im "net=odtest;tables=testodmatrix-nonsparse.csv" --om "net=odtestoutnonsparse" "radii=n;odmatrix" >NUL 2>>testout_od.txt
%pythonexe% ..\..\..\arcscripts\shp2txt.py odtestoutsparse odtestoutnonsparse >>testout_od.txt 2>&1
@echo on
diff -q -w correctout_od.txt testout_od.txt

%pythonexe% -u 3d_test.py >testout3d.txt 2>&1
diff -q -w correctout3d.txt testout3d.txt 
%pythonexe% -u partial_test.py >testout_partial.txt 2>&1
diff -q -w correctout_partial.txt testout_partial.txt 

%pythonexe% -u test_parallel_results.py 

cmd /C run_geom_test.bat
cmd /C run_prepare_test.bat
cmd /C run_prepare_barns_test.bat