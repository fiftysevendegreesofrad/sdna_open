@echo off
sed 's/_%outputsuffix%//g' <%2 >%2.fordiff.txt
cmd /C colourdiff %1 %2.fordiff.txt 