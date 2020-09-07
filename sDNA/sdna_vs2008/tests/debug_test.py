from __future__ import print_function
import ctypes
import sys,os

#sys.stdin.readline()

# http://stackoverflow.com/questions/17840144/why-does-setting-ctypes-dll-function-restype-c-void-p-return-long
class my_void_p(ctypes.c_void_p):
    pass

sdnadll = os.environ["sdnadll"]
dll = ctypes.windll.LoadLibrary(sdnadll)

dll.run_unit_tests()
print()

# now using strings rather than enum so this looks a bit tautological, but:
(ANGULAR, EUCLIDEAN, CUSTOM, HYBRID) = ("ANGULAR","EUCLIDEAN","CUSTOM","HYBRID")

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

def add_polyline(net,arcid,points,_gs,activity_weight=1,custom_cost=1,oneway=0):
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
    if oneway!=0:
        dll.net_add_polyline_data(net,arcid,b"oneway",ctypes.c_float(oneway))

def oneway_test(net):
    add_polyline(net,10,[(0,3),(0,0),(1,0),(1,1),(2,1),(2,0),(3,0),(3,1),(4,1),(4,0),(5,0)],(0,0),1,1,-1)
    add_polyline(net,11,[(0,3),(10,3),(10,2),(5,2),(5,0)],(0,0),1,2,1)
    add_polyline(net,12,[(0,3),(0,103)],(0,0),1,1,0)
    add_polyline(net,13,[(5,0),(105,0)],(0,0),1,1,0)

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

def zero_length_link_test(net):
    add_polyline(net,1,[(0,0),(0,1)],(0,0))
    add_polyline(net,2,[(0,1),(0,1)],(0,0))
    add_polyline(net,3,[(0,1),(0,2)],(0,0))

zero_length_link_test_radii = ["n"]

def disconn_zero_test(net):
    add_polyline(net,1,[(0,0),(0,1)],(0,0))
    add_polyline(net,2,[(2,2),(2,2)],(0,0))

def grid_test(size):
    my_id = 0
    for x in range(size):
        for y in range(size):
            add_polyline(my_id,[(x,y),(x+1,y)],(0,0))
            my_id += 1
            add_polyline(my_id,[(x,y),(x,y+1)],(0,0))
            my_id += 1

def test_net(net_definition,euclidean_radii,analysis_type,cont_space,length_weight,prob_link,extra_config=""):
    global current_net_arcids
    current_net_arcids = []

    dll.net_create.restype = my_void_p
    net = dll.net_create()
    
    config_string = "start_gs=start_gs;end_gs=end_gs;weight=weight;probroutes;outputsums;radii=%s;"%",".join(map(str,euclidean_radii))
    config_string += "metric=%s;"%analysis_type
    config_string += "cont=%s;"%cont_space
    if (analysis_type==CUSTOM):
        config_string += "custommetric=custom_cost;"
    if (analysis_type==HYBRID):
        config_string += "custommetric=custom_cost;" # to check it doesn't get 'wanted' twice
    if length_weight:
        config_string += "weight_type=length;"
    else:
        config_string += "weight_type=link;"
    config_string += "ignorenonlinear;"
    config_string += extra_config
    dll.integral_calc_create.restype = my_void_p
    calculation = dll.integral_calc_create(net,ctypes.c_char_p(config_string.encode("ascii")),
                                                           set_progressor_callback,
                                                           warning_callback)
    print("calc created with config",config_string)
    
    dll.icalc_get_all_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    dll.icalc_get_output_length.restype = ctypes.c_int
    outlength = dll.icalc_get_output_length(calculation)
    names = dll.icalc_get_all_output_names(calculation)
    names = [str(x,"ascii") for x in names[0:outlength]]
    dll.icalc_get_short_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    shortnames = dll.icalc_get_short_output_names(calculation)
    sn=[]
    for i in range(outlength):
        sn += [str(shortnames[i],"ascii")]

    dll.calc_expected_data_net_only.restype = ctypes.c_int
    expected_names_no = ctypes.POINTER(ctypes.c_char_p)()
    expected_names_no_length = dll.calc_expected_data_net_only(calculation,ctypes.byref(expected_names_no))
    expected_names_no = [str(x,"ascii") for x in expected_names_no[0:expected_names_no_length]]
    print("expected names net only:",expected_names_no)
    
    net_definition(net)

    success = dll.calc_run(calculation)
    
    dll.net_print(net)
    
    if success:
        print('\nshortnames: '+','.join(sn))
        print('created output buffer size float *',outlength)

        print('\nOUTPUT DATA:')

        # for debug display we want to invert output arrays
        out_buffer_type = ctypes.c_float * outlength
        out_buffer = out_buffer_type()
        out_array = []
        for i in current_net_arcids:
            dll.icalc_get_all_outputs(calculation,out_buffer,i)
            out_array += [list(out_buffer)]

        for i in range(outlength):
            print(names[i]+' '*(35-len(names[i]))+'  '.join("%.6g"%link_data[i] for link_data in out_array))

        print('\n')

    print('destroying')
    dll.net_destroy(net)
    dll.calc_destroy(calculation)
    print('done')
    print('\n')
    
def test_net_all_options(test_name,net_definition,euclidean_radii,problink,do_weighted):
    print("%s, Angular analysis, discrete space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,False,False,problink)
    print("\n\n%s, Angular analysis, cont space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,True,False,problink)
    print("\n\n%s, Euclidean analysis, discrete space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,EUCLIDEAN,False,False,problink)
    print("\n\n%s, Euclidean analysis, cont space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,EUCLIDEAN,True,False,problink)

    if do_weighted:
        print("%s, Angular analysis, discrete space, weighted"%test_name)
        test_net(net_definition,euclidean_radii,ANGULAR,False,True,problink)
        print("\n\n%s, Angular analysis, cont space, weighted"%test_name)
        test_net(net_definition,euclidean_radii,ANGULAR,True,True,problink)
        print("\n\n%s, Euclidean analysis, discrete space, weighted"%test_name)
        test_net(net_definition,euclidean_radii,EUCLIDEAN,False,True,problink)
        print("\n\n%s, Euclidean analysis, cont space, weighted"%test_name)
        test_net(net_definition,euclidean_radii,EUCLIDEAN,True,True,problink)
        print("\n\n")

    print("%s, Custom analysis, discrete space"%test_name)
    test_net(net_definition,euclidean_radii,CUSTOM,False,False,problink)
    print("\n\n%s, Custom analysis, cont space"%test_name)
    test_net(net_definition,euclidean_radii,CUSTOM,True,False,problink)

    
def test_net_non_strict(test_name,net_definition,euclidean_radii,problink):
    print("%s, Non-strict, Angular analysis, discrete space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,False,False,problink,"nostrictnetworkcut;probrouteaction=ignore")
    print("\n\n%s, Non-strict, Angular analysis, cont space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,True,False,problink,"nostrictnetworkcut;probrouteaction=ignore")
    print("\n\n%s, Non-strict, Euclidean analysis, discrete space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,EUCLIDEAN,False,False,problink,"nostrictnetworkcut;probrouteaction=ignore")
    print("\n\n%s, Non-strict, Euclidean analysis, cont space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,EUCLIDEAN,True,False,problink,"nostrictnetworkcut;probrouteaction=ignore")

    print("%s, Non-strict rerouting, Angular analysis, discrete space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,False,False,problink,"nostrictnetworkcut;probroutethreshold=1;probrouteaction=reroute")
    print("\n\n%s, Non-strict rerouting, Angular analysis, cont space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,True,False,problink,"nostrictnetworkcut;probroutethreshold=1;probrouteaction=reroute")
    print("\n\n%s, Non-strict rerouting, Euclidean analysis, discrete space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,EUCLIDEAN,False,False,problink,"nostrictnetworkcut;probroutethreshold=1;probrouteaction=reroute")
    print("\n\n%s, Non-strict rerouting, Euclidean analysis, cont space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,EUCLIDEAN,True,False,problink,"nostrictnetworkcut;probroutethreshold=1;probrouteaction=reroute")

    print("%s, Non-strict discarding, Angular analysis, discrete space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,False,False,problink,"nostrictnetworkcut;probroutethreshold=1;probrouteaction=discard")
    print("\n\n%s, Non-strict discarding, Angular analysis, cont space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,ANGULAR,True,False,problink,"nostrictnetworkcut;probroutethreshold=1;probrouteaction=discard")
    print("\n\n%s, Non-strict discarding, Euclidean analysis, discrete space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,EUCLIDEAN,False,False,problink,"nostrictnetworkcut;probroutethreshold=1;probrouteaction=discard")
    print("\n\n%s, Non-strict discarding, Euclidean analysis, cont space, unweighted"%test_name)
    test_net(net_definition,euclidean_radii,EUCLIDEAN,True,False,problink,"nostrictnetworkcut;probroutethreshold=1;probrouteaction=discard")

test_net_all_options("Choice Test",choice_test,choice_test_radii,12,True)
test_net_non_strict("Choice Test",choice_test,choice_test_radii,12)

print("Junction cost test")
test_net(junction_cost_test,junction_cost_test_radii,EUCLIDEAN,False,False,1)

print("Split link test")
test_net(split_link_test,split_link_test_radii,EUCLIDEAN,False,False,1)

print("Activity weight test")
test_net(act_weight_test,act_weight_test_radii,EUCLIDEAN,False,False,10)

test_net_all_options("Global Radius Only Test",choice_test,["n"],12,True)

print("Nasty link test")
test_net(nasty_link_test,nasty_link_test_radii,ANGULAR,False,False,1)

print("Triangle stick test")
test_net(triangle_stick_test,triangle_stick_test_radii,EUCLIDEAN,False,False,1)

print("Disconnected zero length link test")
test_net(disconn_zero_test,["n"],ANGULAR,False,False,1)

print("Bidir test, Angular analysis, cont space, unweighted")
test_net(oneway_test,choice_test_radii,ANGULAR,True,False,12,"bidir")

print("Oneway test, Angular analysis, cont space, unweighted")
test_net(oneway_test,choice_test_radii,ANGULAR,True,False,12,"bidir;oneway=oneway")

test_net_all_options("Zero length link test",zero_length_link_test,zero_length_link_test_radii,1,False)
