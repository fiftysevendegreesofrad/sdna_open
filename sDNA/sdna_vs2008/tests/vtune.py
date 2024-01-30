import ctypes
import sys
import numpy
import time

try:
    xrange #type: ignore
except NameError:
    xrange = range

dll = ctypes.windll.LoadLibrary(r"..\vtune\sDNA_vs2008.dll")

(ANGULAR, EUCLIDEAN) = map(ctypes.c_int,xrange(2))

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

def add_polyline(net,arcid,points,elev):
    global current_net_arcids
    current_net_arcids += [arcid]
    point_array_x = (ctypes.c_double*len(points))()
    point_array_y = (ctypes.c_double*len(points))()
    for i,(x,y) in enumerate(points):
        point_array_x[i] = x
        point_array_y[i] = y
    dll.net_add_polyline(net,arcid,len(points),point_array_x,point_array_y)
    dll.net_add_polyline_data(net,arcid,"startelev",ctypes.c_float(elev[0]))
    dll.net_add_polyline_data(net,arcid,"endelev",ctypes.c_float(elev[1]))
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

    radii_array = (ctypes.c_double*len(euclidean_radii))()
    for i,r in enumerate(euclidean_radii):
        radii_array[i] = r

    net = ctypes.c_void_p(dll.net_create(False))
    calculation = ctypes.c_void_p(dll.integral_calc_create(net,len(radii_array),radii_array,prob_link,analysis_type,cont_space,set_progressor_callback,warning_callback))

    net_definition(net)

    before_time = time.clock()
    dll.calc_run(calculation)
    after_time = time.clock()

    if cont_space:
        print("cont,")
    else:
        print("disc,")
    print("%sms\n" % (after_time-before_time)*1000)

    dll.net_destroy(net)
    dll.calc_destroy(calculation)

desired_num_links = 500
bound_grid_test = lambda n: grid_test(n,5000,desired_num_links)
test_net(bound_grid_test,[400,800,1200,2000,3600,5000],ANGULAR,False,1)
test_net(bound_grid_test,[400,800,1200,2000,3600,5000],ANGULAR,True,1)

