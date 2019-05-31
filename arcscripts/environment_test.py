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

from sdna_environment import *
from distutils import dir_util
import os

class GeomItem():
    def __init__(self,geom,data,arcid):
        self.geom = geom
        self.data = data
        self.id = arcid

def test_env(env,outbase):
    print "Testing ",outbase
    print "writing an empty shape"
    cc = env.GetCreateCursor(outbase+"/testemptyshape",["dataname"],["dataname"],"POLYLINE",1)
    cc.AddRowGeomItem(GeomItem([],[1],1))
    cc.AddRowGeomItem(GeomItem([[(0,0),(1,1)]],[0],0))
    cc.Close()
    print "writing an empty shape part"
    cc = env.GetCreateCursor(outbase+"/testemptyshapepart",["dataname"],["dataname"],"POLYLINE",1)
    cc.AddRowGeomItem(GeomItem([[]],[2],2))
    cc.AddRowGeomItem(GeomItem([[(0,0),(1,1)]],[0],0))
    cc.Close()
    print "writing a shape with an empty part"
    cc = env.GetCreateCursor(outbase+"/testshapewithemptypart",["dataname"],["dataname"],"POLYLINE",1)
    cc.AddRowGeomItem(GeomItem([[(0,0),(1,1)],[]],[3],3))
    cc.AddRowGeomItem(GeomItem([[(0,0),(1,1)]],[0],0))
    cc.Close()
    print "writing an empty shape alone"
    cc = env.GetCreateCursor(outbase+"/testloneemptyshape",["dataname"],["dataname"],"POLYLINE",1)
    cc.AddRowGeomItem(GeomItem([],[1],1))
    cc.Close()
    print "writing an empty shape part alone"
    cc = env.GetCreateCursor(outbase+"/testloneemptyshapepart",["dataname"],["dataname"],"POLYLINE",1)
    cc.AddRowGeomItem(GeomItem([[]],[2],2))
    cc.Close()
    print "writing a shape with an empty part alone"
    cc = env.GetCreateCursor(outbase+"/testloneshapewithemptypart",["dataname"],["dataname"],"POLYLINE",1)
    cc.AddRowGeomItem(GeomItem([[(0,0),(1,1)],[]],[3],3))
    cc.Close()


    print "using a 2d geom item create cursor nicely"
    cc = env.GetCreateCursor(outbase+"/test2",["dataname"],["dataname"],"POLYLINE",1)
    cc.AddRowGeomItem(GeomItem([[(0,0),(1,1)]],[0],0))
    cc.Close()
    print "using a 2d normal create cursor nicely"
    cc = env.GetCreateCursor(outbase+"/testnormalcc2d",["dataname"],["dataname"],"POLYLINE")
    cc.AddRow([(0,0),(1,1)],[0])
    cc.Close()
    print "using a 3d normal create cursor nicely"
    cc = env.GetCreateCursor(outbase+"/testnormalcc3d",["dataname"],["dataname"],"POLYLINEZ")
    cc.AddRow([(0,0,0),(1,1,1)],[0])
    cc.Close()
    
    print "using a create cursor geomitem with no data"
    cc = env.GetCreateCursor(outbase+"/ccginodata",[],[],"POLYLINE",0)
    cc.AddRowGeomItem(GeomItem([[(0,0),(1,1)]],[],0))
    cc.Close()
    print
    print

    print "using a create cursor addrow with no data"
    cc = env.GetCreateCursor(outbase+"/ccarnodata",[],[],"POLYLINE",0)
    cc.AddRow([(0,0),(1,1)],[])
    cc.Close()
    print
    print

    print "using a 3d create cursor nicely"
    cc = env.GetCreateCursor(outbase+"/test3dcreate",["dataname"],["dataname"],"POLYLINEZ",1)
    cc.AddRowGeomItem(GeomItem([[(0,0,0),(1,1,1)]],[0],0))
    cc.Close()
    
if os.path.exists("shptest"):
    dir_util.remove_tree("shptest")
dir_util.mkpath("shptest")
test_env(SdnaShapefileEnvironment("shptest/editout",True),"shptest")

if os.path.exists("gdbtest_actual.gdb"):
    dir_util.remove_tree("gdbtest_actual.gdb")
dir_util.copy_tree("gdbtest.gdb","gdbtest_actual.gdb")
test_env(SdnaArcpyEnvironment("",False),"gdbtest_actual.gdb")
