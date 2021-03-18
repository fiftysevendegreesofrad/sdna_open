#sDNA software for spatial network analysis 
#Copyright (C) 2011-2019 Cardiff University

#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <https://www.gnu.org/licenses/>.

import sys
import arcpy

arcpy.AddMessage(sys.executable)

import ctypes

arcpy.AddMessage(ctypes.sizeof(ctypes.c_void_p))

import subprocess
cmd = r"d:\sdna\arcscripts\rportable\R-Portable\App\R-Portable\bin\RScript.exe --no-site-file --no-init-file --no-save --no-environ --no-init-file --no-restore --no-Rconsole test.R"
process = subprocess.Popen(cmd,shell=False,stdout=subprocess.PIPE) 
stdout,stderr = process.communicate()

def output(x):
    for line in x.split("\n"):
        arcpy.AddMessage(line)

output(stdout)
output(stderr)
    
