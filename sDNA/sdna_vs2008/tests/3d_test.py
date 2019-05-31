import sys,arcscriptsdir
from sdnapy import *

#sys.stdin.readline()

try:
    sdnadll = os.environ["sdnadll"]
except:
    sdnadll = r"..\x64\release\sdna_vs2008.dll"

set_dll_path(sdnadll)

def message_callback(x):
    print "sDNA Message:",x
    return 0
set_sdnapy_message_callback(message_callback)
def progress_callback(x):
    return 0

def print_net(net,data=""):
    print net.toString(data=data)
    

print "Grade separation processing in prepare split link test"
net = Net()
net.add_polyline(0,[(0,0,0),(1,0,1)])
net.add_polyline(1,[(1,0,0),(2,0,0)])
net.add_polyline(2,[(1,0,1),(1,1,3)])

net.add_polyline_data(0,"ee",1)
net.add_polyline_data(2,"se",1)
net.add_polyline_data(2,"ee",3)

print "before"
print_net(net,"se,ee")
pc = Calculation("sdnaprepare","action=repair;splitlinks;start_gs=se;end_gs=ee",net,progress_callback,message_callback)
pc.run()
print "after"
print pc.toString()

print

print "3d Clustering prepare test"
net = Net()
# cluster 1
net.add_polyline(0,[(0,0,0),(1,0,0)]) 
net.add_polyline(1,[(0,0,0),(0.9,0,0)])
net.add_polyline(2,[(0,0,0),(1,0,0.01)])
# cluster 2
net.add_polyline(3,[(0,0,0),(1,0,1)])
net.add_polyline(4,[(0,0,0),(1,0,0.99)])
net.add_polyline(5,[(0,0,0),(0.9,0,1)])
net.add_polyline(6,[(0,0,0),(0.9,0,0.99)])
# unclustered
net.add_polyline(7,[(0,0,0),(2,0,0)]) # matching z not xy with cluster 1
net.add_polyline(8,[(0,0,0),(2,0,1)]) # matching z not xy with cluster 2
net.add_polyline(9,[(0,0,0),(1,0,0.9)]) # closer than xytol but only in z to cluster 2
#also cluster 1 and 2 match in xy but not z
for i in range(10):
    net.add_polyline_data(i,"test_input_id",i)
print "before"
print_net(net)
pc = Calculation("sdnaprepare","action=repair;nearmisses;arcxytol=0.2;ztol=0.02;data_absolute=test_input_id",net,progress_callback,message_callback)
pc.run()
print "after"
print pc.toString()

print

print "Vertical euclidean network test"
# in which we rotate a network to the x-z plane and check results match those of the
# same network in the x-y plane
def verticalize(points):
    return [(x,0,y) for x,y in points]

points_10 = [(0,3),(0,0),(1,0),(1,1),(2,1),(2,0),(3,0),(3,1),(4,1),(4,0),(5,0)]
points_11 = [(0,3),(10,3),(10,2),(5,2),(5,0)]
points_12 = [(0,3),(0,103)]
points_13 = [(5,0),(105,0)]
net = Net()
net2 = Net()
net.add_polyline(10,points_10)
net.add_polyline(11,points_11)
net.add_polyline(12,points_12)
net.add_polyline(13,points_13)
net2.add_polyline(10,verticalize(points_10))
net2.add_polyline(11,verticalize(points_11))
net2.add_polyline(12,verticalize(points_12))
net2.add_polyline(13,verticalize(points_13))
config = "metric=EUCLIDEAN;nohull;radii=20,n;cont"
ic1 = Calculation("sdnaintegral",config,net,progress_callback,message_callback)
ic2 = Calculation("sdnaintegral",config,net2,progress_callback,message_callback)
ic1.run()
ic2.run()
out1 = ic1.get_geom_outputs().next()
out2 = ic2.get_geom_outputs().next()
names1 = out1.shortdatanames
names2 = out2.shortdatanames
success = True
if names1 != names2:
    print "Names don't match!"
    print names1
    print names2
    success = False
else:
    print "Names match",names1
    for link1,link2 in zip(out1.get_items(),out2.get_items()):
        outputs1 = link1.data
        outputs2 = link2.data
        if outputs1 != outputs2:
            for i,(o1,o2) in enumerate(zip(outputs1,outputs2)):
                if o1 != o2:
                    print "Unmatched output %s for link %d: %f, %f"%(names1[i],outputs1[0],o1,o2)
                    success = False
if success:
    print "All outputs match"

del net,net2,ic1,ic2
print

print "Vertical step in link having correct 3d angular cost test"
print "Warning should be produced that vertical link segments have ambiguous 2d angular costs so we use zero"
net = Net()
net.add_polyline(0,[(0,0,0),(1,0,0),(1,0,1),(2,0,1)])
print_net(net)
ic = Calculation("sdnaintegral","metric=ANGULAR",net,progress_callback,message_callback)
ic.run()
geom = ic.get_geom_outputs().next()
names = geom.shortdatanames
link0data = geom.get_items().next().data
results = dict(zip(names,link0data))
print "Angular cost of stepped link = ",results["LAC"]
print

print "Zero length link must warn of this when above tests didn't"
net = Net()
from sys import float_info
m = float_info.min
net.add_polyline(0,[(0,0,0),(m,0,0)])
print_net(net)
ic = Calculation("sdnaintegral","metric=ANGULAR",net,progress_callback,message_callback)
ic.run()
print

print "Fix split links on 3d data shouldn't create _gs fields"
net = Net()
net.add_polyline(0,[(0,0,0),(1,1,1)])
net.add_polyline(1,[(2,2,2),(1,1,1)])
p = Calculation("sdnaprepare","action=repair;splitlinks",net,progress_callback,message_callback)
p.run()
print p.toString()
print

print "Disable link test"
net = Net()
net.add_polyline(0,[(0,0,0),(1,1,1)])
net.add_polyline(1,[(2,2,2),(1,1,1)])
net.add_polyline(2,[(0,0,0),(-1,-1,-1)])
net.add_polyline_data(1,"disable",1)
p = Calculation("sdnaintegral","disable=disable",net,progress_callback,message_callback)
p.run()
print p.toString()
