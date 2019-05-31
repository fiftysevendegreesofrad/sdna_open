@echo off

echo running shape tests

del tinyout*

%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py -i tiny.shp -o tinyout_1_10_cont --dll %sdnadll% "metric=EUCLIDEAN;outputgeodesics;nonetdata;radii=350;origins=1;destinations=10;cont" >NUL 

%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py -i tiny.shp -o tinyout_12_47_bothends_strict --dll %sdnadll% "metric=ANGULAR;outputgeodesics;nonetdata;radii=350;origins=12;destinations=47;cont" >NUL 

%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py -i tiny.shp -o tinyout_12_47_bothends_nonstrict --dll %sdnadll% "metric=ANGULAR;outputgeodesics;nonetdata;radii=350;origins=12;destinations=47;cont;nostrictnetworkcut" >NUL 

%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py -i tiny.shp -o tinyout_originfrommiddle --dll %sdnadll% "metric=ANGULAR;outputdestinations;nonetdata;radii=100;origins=3;destinations=3;cont" >NUL 

%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py -i tiny.shp -o tinyout_all --dll %sdnadll% "metric=ANGULAR;outputdestinations;outputnetradii;outputgeodesics;outputhulls;nonetdata;radii=5,300,n;origins=16;cont" >NUL 

%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py -i tiny.shp -o tinyout_radmetcomp --dll %sdnadll% "metric=ANGULAR;outputgeodesics;nonetdata;radii=200;origins=16" >NUL 

%pythonexe% -u ..\..\..\arcscripts\bin\sdnaintegral.py -i tiny.shp -o tinyout_radmeteucsim --dll %sdnadll% "metric=ANGULAR;outputgeodesics;nonetdata;radii=400;origins=16;radmetric=hybrid;radlineformula=2*LLen*euc/FULLeuc" >NUL 

%pythonexe% -u ..\..\..\arcscripts\shp2txt.py tinyout_1_10_cont_* tinyout_1_10_disc_* tinyout_12_47_bothends_strict_* tinyout_12_47_bothends_nonstrict_* tinyout_originfrommiddle_* tinyout_all_* tinyout_radmet* >testout_geom.txt 

@echo on

diff -q -w correctout_geom.txt testout_geom.txt



