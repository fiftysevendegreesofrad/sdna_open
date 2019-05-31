import ctypes
import sys,os,re
import numpy
import time
import arcscriptsdir
import sdnapy

#print "attach debugger"
#sys.stdin.readline()

serial_dll_path = os.environ["sdnadll"]
parallel_dll_path = re.sub("debug","parallel_debug",serial_dll_path,flags=re.IGNORECASE)

if os.path.getmtime(serial_dll_path)-os.path.getmtime(parallel_dll_path) > 3600*10:
    print "dlls built more than 10 hours apart"
    sys.exit(1)

#print "serial dll",serial_dll_path
#print "parallel dll",parallel_dll_path

parallel_dll = ctypes.windll.LoadLibrary(parallel_dll_path)
serial_dll = ctypes.windll.LoadLibrary(serial_dll_path)

current_net_arcids = None

# dummy progress bar callback func
CALLBACKFUNCTYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_long)
def set_progressor(x):
    return 0
set_progressor_callback = CALLBACKFUNCTYPE(set_progressor)
WARNINGCALLBACKFUNCTYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_char_p)
def warning(x):
    #print x
    return 0
warning_callback = WARNINGCALLBACKFUNCTYPE(warning)


def add_polyline(dll,net,arcid,points,elev):
    global current_net_arcids
    current_net_arcids += [arcid]
    point_array_x = (ctypes.c_double*len(points))()
    point_array_y = (ctypes.c_double*len(points))()
    for i,(x,y) in enumerate(points):
        point_array_x[i] = x
        point_array_y[i] = y
    dll.net_add_polyline(net,arcid,len(points),point_array_x,point_array_y)
    dll.net_add_polyline_data(net,arcid,"arcid",ctypes.c_float(arcid))
    dll.net_add_polyline_text_data(net,arcid,"zone",ctypes.c_char_p("%d"%(arcid%1)))
    
def add_random_kink_link(dll,net,my_id,squaresize,origin,destination):
    point_list = [origin,destination]
    num_segs = int(numpy.random.exponential(2.15))
    for _ in range(num_segs):
        kink_point = (origin+destination)/2+numpy.random.random(2)/2
        point_list.insert(1,kink_point)

    point_list = [point*squaresize for point in point_list]
    add_polyline(dll,net,my_id,point_list,(0,0))

def grid_test(dll,net,net_size,desired_num_links):
    numpy.random.seed(seed=13)
    squares_per_side = int(pow(desired_num_links/2,0.5))
    grid_square_size = net_size/squares_per_side
    my_id = 0
    for x in range(squares_per_side):
        for y in range(squares_per_side):
            add_random_kink_link(dll,net,my_id,grid_square_size,numpy.array((x,y)),numpy.array((x+1,y)))
            my_id += 1
            add_random_kink_link(dll,net,my_id,grid_square_size,numpy.array((x,y)),numpy.array((x,y+1)))
            my_id += 1

def test_net(dll,net_definition,euclidean_radii,cont_space,prob_link):

    global current_net_arcids
    current_net_arcids = []

    net = ctypes.c_void_p(dll.net_create())

    config_string = "radii=%s;"%",".join(map(str,euclidean_radii))
    config_string += "metric=euclidean_angular;outputskim;skimzone=zone;"
    config_string += "cont=%s;"%cont_space
    
    # can't set restype as c_void_p or it truncates 64 bit addresses
    calculation = ctypes.c_void_p(dll.integral_calc_create(net,config_string,set_progressor_callback,warning_callback))
    if not calculation:
        print "calc create failed"
        sys.exit(0)
 
    net_definition(dll,net)

    dll.calc_run.restype = ctypes.c_int
    savestdout = os.dup(1)
    os.close(1)
    calcres = dll.calc_run(calculation)
    os.dup(savestdout)
    if not calcres:
        print "calc run failed"
        sys.exit(0)
 
    dll.icalc_get_short_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    dll.icalc_get_output_length.restype = ctypes.c_int
    outlength = dll.icalc_get_output_length(calculation)
    names = dll.icalc_get_short_output_names(calculation)
    n=[]
    for i in range(outlength):
        n += [names[i]]
    
    out_buffer_type = ctypes.c_float * outlength
    out_buffer = out_buffer_type()

    out_array = []
    for i in current_net_arcids:
        dll.icalc_get_all_outputs(calculation,out_buffer,i)
        out_array += [list(out_buffer)]
        
    dll.calc_get_num_geometry_outputs.restype = ctypes.c_long
    numgeomouts = dll.calc_get_num_geometry_outputs(calculation)
    for i in range(numgeomouts):
        geom = sdnapy.GeometryLayer(ctypes.c_void_p(dll.calc_get_geometry_output(calculation,ctypes.c_long(i))))
        if geom.name=="skim":
            skimdata = [item.data for item in geom.get_items()]
            
    dll.net_destroy(net)
    dll.calc_destroy(calculation)

    return n,out_array,skimdata

desired_num_links = 50
bound_grid_test = lambda d,n: grid_test(d,n,5000,desired_num_links)

start = time.time()
print "testing serial"
snames,sdata,sskim = test_net(serial_dll,bound_grid_test,[400,1000,"n"],True,1)
serial_end = time.time()
print serial_end-start,"secs"
print "testing parallel"
parallel_start = time.time()
pnames,pdata,pskim = test_net(parallel_dll,bound_grid_test,[400,1000,"n"],True,1)
parallel_end = time.time()
print parallel_end-parallel_start,"secs"
print "though most of this is io so times will be very similar"
assert(pnames==snames)

all_matches = True
for link in current_net_arcids:
    for i,name in enumerate(pnames):
        if str(pdata[link][i])!=str(sdata[link][i]):
            print link,name,sdata[link][i],pdata[link][i]
            all_matches = False

for sd,pd in zip(sskim,pskim):
    if sd!=pd:
        print "skim mismatch: "
        print sd
        print pd
        all_matches = False

assert(all_matches)    
       
print "Serial and parallel results match"
