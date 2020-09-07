import arcscriptsdir
from sdna_environment import *

env = SdnaShapefileEnvironment()
cc=env.GetCreateCursor("odtest",[b"origzone",b"destzone"],[b"origzone",b"destzone"],[b"STR",b"STR"],b"POLYLINEZ")
cc.StartProgressor(24)

class LineGeomItem(object):
    def __init__(self,line,text1,text2):
        self.geom = [line]
        self.data = [text1,text2]
        
# origins        
cc.AddRowGeomItem(LineGeomItem([(0,0),(0,1)],"a","x"))
cc.AddRowGeomItem(LineGeomItem([(1,0),(1,1)],"a","x"))
cc.AddRowGeomItem(LineGeomItem([(2,0),(2,1)],"c","x"))
cc.AddRowGeomItem(LineGeomItem([(3,0),(3,1)],"c","x"))

# destinations
cc.AddRowGeomItem(LineGeomItem([(0,2),(0,3)],"x","a"))
cc.AddRowGeomItem(LineGeomItem([(1,2),(1,3)],"x","a"))
cc.AddRowGeomItem(LineGeomItem([(2,2),(2,3)],"x","b"))
cc.AddRowGeomItem(LineGeomItem([(3,2),(3,3)],"x","b"))

# links
for orig in range(4):
    for dest in range(4):
        cc.AddRowGeomItem(LineGeomItem([(orig,1),(dest,2)],"",""))
        
cc.Close()
