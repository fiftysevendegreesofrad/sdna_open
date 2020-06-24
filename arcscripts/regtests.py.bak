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

from sdnaregutilities import *
import numpy

def simdata():
    xs = numpy.arange(30)
    ys = xs + 5*numpy.sin(xs)
    return xs,ys

xs,ys = simdata()
print '''
0.925476862734
(0.9802520605214966, 0.49769211916578016)
'''

print pearson_r(xs,ys,[1 for x in xs])
print univar_regress(xs,ys,[1 for x in xs])
