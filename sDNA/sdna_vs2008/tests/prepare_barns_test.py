from make_barnsbury import *
import sys,os
from collections import defaultdict
import arcscriptsdir
import sdnapy

#sys.stdin.readline()

def warning(x):
    print (x)
    return 0
warning_callback = warning

dllpath = os.environ["sdnadll"]
print ("dll is",dllpath)
sdnapy.set_dll_path(dllpath)

def add_polyline_inner(net,arcid,points,_gs,weight,cost,isisland):
        net.add_polyline(arcid,points)
        net.add_polyline_data(arcid,"start_gs",_gs[0])
        net.add_polyline_data(arcid,"end_gs",_gs[1])
        net.add_polyline_data(arcid,"weight",weight)
        net.add_polyline_data(arcid,"cost",cost)
        net.add_polyline_data(arcid,"island",isisland)

def add_polyline(net,arcid,points,start_gs,end_gs,isisland):
    add_polyline_inner(net,arcid,points,(start_gs,end_gs),1,1,isisland)

print ("full barnsbury prepare test")
net = sdnapy.Net()
net.add_edge = lambda arcid,points,se,ee,i: add_polyline(net,arcid,points,se,ee,i)
make_barnsbury(net)
prep = sdnapy.Calculation("sdnaprepare","start_gs=start_gs;end_gs=end_gs;island=island;islandfieldstozero=weight,cost;action=detect;splitlinks;trafficislands;duplicates;isolated",net,warning_callback,warning_callback)
prep.run()
results = list(prep.get_geom_outputs())[0]
errors = []
for item in results.get_items():
    errors += [item.data]
print ("split links")
print ([arcid for arcid,errtype in errors if errtype=="Split Link"])
print ("traffic islands")
print ([arcid for arcid,errtype in errors if errtype=="Traffic Island"])
print ("duplicates")
print ([arcid for arcid,errtype in errors if errtype=="Duplicate"])
print ("isolated systems")
print ([arcid for arcid,errtype in errors if errtype=="Isolated"])
print ("fixing")
prep = sdnapy.Calculation("sdnaprepare","start_gs=start_gs;end_gs=end_gs;island=island;islandfieldstozero=weight,cost;action=repair;splitlinks;trafficislands;duplicates;isolated",net,warning_callback,warning_callback)
prep.run()
print ("done")

net2 = sdnapy.Net()
net2.add_edge = lambda arcid,points,se,ee,i: add_polyline(net2,arcid,points,se,ee,i)
make_barnsbury(net2)
print ("before")
print (net2.toString())
print ("after")
prepout = prep.toString()
import re
for line in prepout.split("\n"):
    m=re.match("(\[\('ID'[^\]]*\]) (.*)",line)
    if m:
        data,points = m.groups()
        data = dict(eval(data))
        print ("item %s %s ( %s , %s ) w= %s  c= %s"%(data["ID"],points,
                                                             data["start_gs"],
                                                             data["end_gs"],
                                                             data["weight"],
                                                             data["cost"]))
    else:
        print (line)
        
