import ctypes
import sys
import numpy
import time
import os

sdnadll = os.environ["sdnadll"]
print "dll name",sdnadll

dll = ctypes.windll.LoadLibrary(sdnadll)

current_net_arcids = None

# dummy progress bar callback func
CALLBACKFUNCTYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_long)
def set_progressor(x):
    return 0
set_progressor_callback = CALLBACKFUNCTYPE(set_progressor)
WARNINGCALLBACKFUNCTYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_char_p)
def warning(x):
    return 0
warning_callback = WARNINGCALLBACKFUNCTYPE(warning)

def add_polyline(net,arcid,points,_gs):
    global current_net_arcids
    current_net_arcids += [arcid]
    point_array_x = (ctypes.c_double*len(points))()
    point_array_y = (ctypes.c_double*len(points))()
    for i,(x,y) in enumerate(points):
        point_array_x[i] = x
        point_array_y[i] = y
    dll.net_add_polyline(net,arcid,len(points),point_array_x,point_array_y)
    dll.net_add_polyline_data(net,arcid,"start_gs",ctypes.c_float(_gs[0]))
    dll.net_add_polyline_data(net,arcid,"end_gs",ctypes.c_float(_gs[1]))
    dll.net_add_polyline_data(net,arcid,"weight",ctypes.c_float(1))
    dll.net_add_polyline_data(net,arcid,"custom_cost",ctypes.c_float(1))
    dll.net_add_polyline_data(net,arcid,"is_island",ctypes.c_float(0))
    
def add_random_kink_link(net,my_id,squaresize,origin,destination):
    point_list = [origin,destination]
    num_segs = int(numpy.random.exponential(2.15))
    for _ in range(num_segs):
        kink_point = (origin+destination)/2+numpy.random.random(2)/2
        point_list.insert(1,kink_point)

    point_list = [point*squaresize for point in point_list]
    add_polyline(net,my_id,point_list,(0,0))

def grid_test(net,net_size,desired_num_links):
    numpy.random.seed(seed=13)
    squares_per_side = int(pow(desired_num_links/2,0.5))
    grid_square_size = net_size/squares_per_side
    my_id = 0
    for x in range(squares_per_side):
        for y in range(squares_per_side):
            add_random_kink_link(net,my_id,grid_square_size,numpy.array((x,y)),numpy.array((x+1,y)))
            my_id += 1
            add_random_kink_link(net,my_id,grid_square_size,numpy.array((x,y)),numpy.array((x,y+1)))
            my_id += 1

def test_net(net_definition,euclidean_radii,analysis_type,cont_space,prob_link):

    global current_net_arcids
    current_net_arcids = []

    config_string = "radii=%s;"%",".join(map(str,euclidean_radii))
    if (cont_space):
        config_string += "cont;"
    config_string += "metric=%s;"%analysis_type
    net = ctypes.c_void_p(dll.net_create())
    
    calculation = ctypes.c_void_p(dll.integral_calc_create(net,config_string,set_progressor_callback,warning_callback))
    
    net_definition(net)

    before_time = time.clock()
    dll.calc_run(calculation)
    after_time = time.clock()

    if cont_space:
        print "cont,",
    else:
        print "disc,",
    print (after_time-before_time)*1000,"ms\n"

    dll.net_destroy(net)
    dll.calc_destroy(calculation)

desired_num_links = 1500
bound_grid_test = lambda n: grid_test(n,5000,desired_num_links)
large_radius_list = [400,800,1200,2000,3600,5000]
small_radius_list = [400,5000]
print "large r list"
test_net(bound_grid_test,large_radius_list,"ANGULAR",False,1)
test_net(bound_grid_test,large_radius_list,"ANGULAR",True,1)
print "small r list"
test_net(bound_grid_test,small_radius_list,"ANGULAR",False,1)
test_net(bound_grid_test,small_radius_list,"ANGULAR",True,1)

