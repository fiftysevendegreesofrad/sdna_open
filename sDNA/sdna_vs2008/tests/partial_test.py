import ctypes
import sys,os

# http://stackoverflow.com/questions/17840144/why-does-setting-ctypes-dll-function-restype-c-void-p-return-long
class my_void_p(ctypes.c_void_p):
    pass

#sys.stdin.readline()

dll = ctypes.windll.LoadLibrary(os.environ["sdnadll"])

# now using strings rather than enum so this looks a bit tautological, but:
(ANGULAR, EUCLIDEAN, CUSTOM) = ("ANGULAR","EUCLIDEAN","CUSTOM")

current_net_arcids = None

# dummy progress bar callback func
CALLBACKFUNCTYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_long)
def set_progressor(x):
    return 0
set_progressor_callback = CALLBACKFUNCTYPE(set_progressor)
WARNINGCALLBACKFUNCTYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_char_p)
def warning(x):
    print(str(x,"ascii"))
    return 0
warning_callback = WARNINGCALLBACKFUNCTYPE(warning)

def add_polyline(net,arcid,points,_gs,activity_weight=1,custom_cost=1):
    global current_net_arcids
    current_net_arcids += [arcid]
    point_array_x = (ctypes.c_double*len(points))()
    point_array_y = (ctypes.c_double*len(points))()
    for i,(x,y) in enumerate(points):
        point_array_x[i] = x
        point_array_y[i] = y
    dll.net_add_polyline(net,arcid,len(points),point_array_x,point_array_y)
    dll.net_add_polyline_data(net,arcid,b"start_gs",ctypes.c_float(_gs[0]))
    dll.net_add_polyline_data(net,arcid,b"end_gs",ctypes.c_float(_gs[1]))
    dll.net_add_polyline_data(net,arcid,b"weight",ctypes.c_float(activity_weight))
    dll.net_add_polyline_data(net,arcid,b"custom_cost",ctypes.c_float(custom_cost))
    dll.net_add_polyline_data(net,arcid,b"is_island",ctypes.c_float(0))

def choice_test(net):
    add_polyline(net,10,[(0,3),(0,0),(1,0),(1,1),(2,1),(2,0),(3,0),(3,1),(4,1),(4,0),(5,0)],(0,0),1,1)
    add_polyline(net,11,[(0,3),(10,3),(10,2),(5,2),(5,0)],(0,0),1,2)
    add_polyline(net,12,[(0,3),(0,103)],(0,0),1,1)
    add_polyline(net,13,[(5,0),(105,0)],(0,0),1,1)

choice_test_radii = [5,6,10,50,63,"n"]

def act_weight_test(net):
    add_polyline(net,10,[(0,3),(0,0),(1,0),(1,1),(2,1),(2,0),(3,0),(3,1),(4,1),(4,0),(5,0)],(0,0))
    add_polyline(net,11,[(0,3),(10,3),(10,2),(5,2),(5,0)],(0,0))
    add_polyline(net,12,[(0,3),(0,103)],(0,0))
    add_polyline(net,13,[(5,0),(105,0)],(0,0),0)

act_weight_test_radii = ["n"]

def variable_center_test(net):
    add_polyline(net,10,[(0,0),(0,1),(-1,1),(-1,0),(-2,0),(-2,1),(-3,1),(-3,0),(-10,0)],(0,0))
    add_polyline(net,11,[(0,0),(0,-7),(-1,-7),(-1,-8),(0,-8),(0,-9),(-1,-9),(-1,-10),(0,-10)],(0,0))
    add_polyline(net,12,[(0,0),(7,0),(7,1),(8,1),(8,0),(9,0),(9,1),(10,1),(10,0)],(0,0))

variable_center_test_radii = [10.5,21,"n"]

def junction_cost_test(net):
    add_polyline(net,1,[(0,0),(-1,0)],(0,0))
    add_polyline(net,2,[(0,0),(1,1)],(0,0))
    add_polyline(net,3,[(0,0),(1,-1)],(0,0))

junction_cost_test_radii = ["n"]

def split_link_test(net):
    add_polyline(net,1,[(0,0),(0,1)],(0,0))
    add_polyline(net,2,[(0,1),(0,2)],(0,0))

split_link_test_radii = ["n"]

def nasty_link_test(net):
    add_polyline(net,1,[(0,0),(0,1),(0,1),(0,2)],(0,0))

nasty_link_test_radii = ["n"]

def singularity_test(net):
    add_polyline(net,1,[(0,0),(0,0)],(0,0))

def triangle_stick_test(net):
    add_polyline(net,1,[(0,1),(1,1)],(0,0))
    add_polyline(net,2,[(1,1),(2,2),(2,0),(1,1)],(0,0))
    add_polyline(net,3,[(10,10),(11,10)],(0,0))
    add_polyline(net,4,[(11,10),(12,10)],(0,0))

triangle_stick_test_radii = ["n"]


def grid_test(size):
    my_id = 0
    for x in range(size):
        for y in range(size):
            add_polyline(my_id,[(x,y),(x+1,y)],(0,0))
            my_id += 1
            add_polyline(my_id,[(x,y),(x,y+1)],(0,0))
            my_id += 1

def test_net(net_definition,euclidean_radii,analysis_type,cont_space,length_weight,prob_link,extra_config):
    global current_net_arcids
    current_net_arcids = []

    dll.net_create.restype = my_void_p
    net = dll.net_create()

    config_string = "start_gs=start_gs;end_gs=end_gs;custommetric=custom_cost;weight=weight;radii=%s;"%",".join(map(str,euclidean_radii))
    config_string += "metric=%s;"%analysis_type
    config_string += "cont=%s;"%cont_space
    if length_weight:
        config_string += "weight_type=length;"
    else:
        config_string += "weight_type=link;"
    config_string += extra_config
    dll.integral_calc_create.restype = my_void_p
    calculation = dll.integral_calc_create(net,ctypes.c_char_p(config_string.encode("ascii")),
                                                           set_progressor_callback,
                                                           warning_callback)

    if not calculation:
        print ("config failed")
        return
    
    dll.icalc_get_all_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    dll.icalc_get_output_length.restype = ctypes.c_int
    outlength = dll.icalc_get_output_length(calculation)
    names = [str(x,"ascii") for x in dll.icalc_get_all_output_names(calculation)[0:outlength]]
    dll.icalc_get_short_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    shortnames = dll.icalc_get_short_output_names(calculation)
    sn=[]
    for i in range(outlength):
        sn += [shortnames[i]]

    net_definition(net)
    
    if not dll.calc_run(calculation):
        print ("run failed")
        return

    dll.net_print(net)
    
    
    print ('\nshortnames: '+','.join([str(x,"ascii") for x in sn]))
    print ('created output buffer size float *',outlength)

    print ('\nOUTPUT DATA:')

    # for debug display we want to invert output arrays
    out_buffer_type = ctypes.c_float * outlength
    out_buffer = out_buffer_type()
    out_array = []
    for i in current_net_arcids:
        dll.icalc_get_all_outputs(calculation,out_buffer,i)
        out_array += [list(out_buffer)]

    for i in range(outlength):
        print (names[i]+' '*(35-len(names[i]))+'  '.join("%.6g"%link_data[i] for link_data in out_array))

    print ()
    print ('destroying')
    dll.net_destroy(net)
    dll.calc_destroy(calculation)
    print ('done')
    print ()
    
def test_net_all_options(test_name,net_definition,euclidean_radii,problink,extra_config):
    print ("%s, Angular analysis, discrete space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,False,False,problink,extra_config)
    print ("\n\n%s, Angular analysis, cont space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,True,False,problink,extra_config)

def bigger_test(extra_config):
    test_net_all_options("Choice Test",choice_test,choice_test_radii,12,extra_config)
    print ("Junction cost test")
    test_net(junction_cost_test,junction_cost_test_radii,EUCLIDEAN,False,False,1,extra_config)
    print ("Split link test")
    test_net(split_link_test,split_link_test_radii,EUCLIDEAN,False,False,1,extra_config)
    print ("Activity weight test")
    test_net(act_weight_test,act_weight_test_radii,EUCLIDEAN,False,False,10,extra_config)
    test_net_all_options("Global Radius Only Test",choice_test,["n"],12,extra_config)
    print ("Nasty link test")
    test_net(nasty_link_test,nasty_link_test_radii,ANGULAR,False,False,1,extra_config)
    print ("Triangle stick test")
    test_net(triangle_stick_test,triangle_stick_test_radii,EUCLIDEAN,False,False,1,extra_config)
    
bigger_test("linkonly")
bigger_test("nobetweenness;nojunctions;nohull")
bigger_test("nojunctions;nohull")
bigger_test("nobetweenness")
