import arcscriptsdir
from sdna_environment import *

env = SdnaShapefileEnvironment()
cc=env.GetCreateCursor("skimtestzeroweight",["zone","weight"],
                       ["zone","weight"],["STR","FLOAT"],"POLYLINEZ")
cc.StartProgressor(2)

class LineGeomItem(object):
    def __init__(self,line,text1):
        self.geom = [line]
        self.data = [text1,0.]
        
# origins        
cc.AddRowGeomItem(LineGeomItem([(0,0),(0,1)],"a"))
cc.AddRowGeomItem(LineGeomItem([(0,1),(0,2)],"b"))
        
cc.Close()
