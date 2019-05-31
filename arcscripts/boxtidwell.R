#!/usr/bin/Rscript
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


yfilename = commandArgs(trailingOnly=TRUE)[1]
xfilename = commandArgs(trailingOnly=TRUE)[2]
x2filename = commandArgs(trailingOnly=TRUE)[3]
y = read.table(yfilename)
x = read.table(xfilename)

x2 = NULL
result <- try(function() x2=read.table(x2filename))

suppressPackageStartupMessages(require(car,quietly=TRUE))

if (is.null(x2))
{
    bt = boxTidwell(as.matrix(y)[,1],x1=as.matrix(x))
} else {
    bt = boxTidwell(as.matrix(y)[,1],x1=as.matrix(x),x2=as.matrix(x2))
}

for (lambda in bt$result[,3]) # third column contains lambda
{
	cat(sprintf("%f\n",lambda))
}
