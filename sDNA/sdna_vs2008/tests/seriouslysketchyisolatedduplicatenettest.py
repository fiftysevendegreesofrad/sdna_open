import ctypes
import sys

#sys.stdin.readline()

dll = ctypes.windll.sDNA

(ANGULAR, EUCLIDEAN) = map(ctypes.c_int,xrange(2))

current_net_arcids = None
offset = 0

# dummy progress bar callback func
CALLBACKFUNCTYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_long)
def set_progressor(x):
    return 0
set_progressor_callback = CALLBACKFUNCTYPE(set_progressor)

def add_link(net,arcid,points,elev):
    global current_net_arcids,offset
    current_net_arcids += [arcid+offset]
    point_array_x = (ctypes.c_double*len(points))()
    point_array_y = (ctypes.c_double*len(points))()
    for i,(x,y) in enumerate(points):
        point_array_x[i] = x+offset
        point_array_y[i] = y+offset
    dll.net_add_link(net,arcid+offset,len(points),
                     point_array_x,point_array_y,
                     ctypes.c_double(elev[0]),ctypes.c_double(elev[1]),False)

def choice_test(net):
    add_link(net,10,[(0,3),(0,0),(1,0),(1,1),(2,1),(2,0),(3,0),(3,1),(4,1),(4,0),(5,0)],(0,0))
    add_link(net,11,[(0,3),(10,3),(10,2),(5,2),(5,0)],(0,0))
    add_link(net,12,[(0,3),(0,103)],(0,0))
    add_link(net,13,[(5,0),(105,0)],(0,0))

choice_test_radii = [5,6,10,50,63,-1]

def variable_center_test(net):
    add_link(net,10,[(0,0),(0,1),(-1,1),(-1,0),(-2,0),(-2,1),(-3,1),(-3,0),(-10,0)],(0,0))
    add_link(net,11,[(0,0),(0,-7),(-1,-7),(-1,-8),(0,-8),(0,-9),(-1,-9),(-1,-10),(0,-10)],(0,0))
    add_link(net,12,[(0,0),(7,0),(7,1),(8,1),(8,0),(9,0),(9,1),(10,1),(10,0)],(0,0))

variable_center_test_radii = [10.5,21,-1]

def junction_cost_test(net):
    add_link(net,1,[(0,0),(-1,0)],(0,0))
    add_link(net,2,[(0,0),(1,1)],(0,0))
    add_link(net,3,[(0,0),(1,-1)],(0,0))

junction_cost_test_radii = [-1]

def grid_test(size):
    my_id = 0
    for x in range(size):
        for y in range(size):
            add_link(my_id,[(x,y),(x+1,y)],(0,0))
            my_id += 1
            add_link(my_id,[(x,y),(x,y+1)],(0,0))
            my_id += 1

def test_net(net_definition,euclidean_radii,analysis_type,cont_space,length_weight,prob_link):
    global current_net_arcids,offset
    current_net_arcids = []

    radii_array = (ctypes.c_double*len(euclidean_radii))()
    for i,r in enumerate(euclidean_radii):
        radii_array[i] = r

    net = ctypes.c_void_p(dll.net_create(length_weight))
    calculation = ctypes.c_void_p(dll.integral_calc_create(net,len(radii_array),radii_array,
                                                           prob_link,analysis_type,
                                                           cont_space,
                                                           set_progressor_callback))

    dll.calc_get_all_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    dll.calc_get_output_length.restype = ctypes.c_int
    outlength = dll.calc_get_output_length(calculation)
    names = dll.calc_get_all_output_names(calculation)
    dll.calc_get_short_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    shortnames = dll.calc_get_short_output_names(calculation)
    sn=[]
    for i in range(outlength):
        sn += [shortnames[i]]

    offset = 0
    net_definition(net)
    
    dll.calc_run(calculation)

    dll.net_print(net)
    
    
    print '\nshortnames: '+','.join(sn)
    print 'created output buffer size double *',outlength

    print '\nOUTPUT DATA:'

    # for debug display we want to invert output arrays
    out_buffer_type = ctypes.c_double * outlength
    out_buffer = out_buffer_type()
    out_array = []
    for i in current_net_arcids:
        dll.calc_get_all_outputs(calculation,out_buffer,i)
        out_array += [list(out_buffer)]

    for i in range(outlength):
        print names[i]+' '*(25-len(names[i]))+'  '.join(str(link_data[i]) for link_data in out_array)

    print '\n'
    print 'destroying'
    dll.net_destroy(net)
    dll.calc_destroy(calculation)
    print 'done'
    print '\n'
    test_net_duplicate(net_definition,euclidean_radii,analysis_type,cont_space,length_weight,prob_link)

def test_net_duplicate(net_definition,euclidean_radii,analysis_type,cont_space,length_weight,prob_link):
    global current_net_arcids,offset
    current_net_arcids = []

    radii_array = (ctypes.c_double*len(euclidean_radii))()
    for i,r in enumerate(euclidean_radii):
        radii_array[i] = r

    net = ctypes.c_void_p(dll.net_create(length_weight))
    calculation = ctypes.c_void_p(dll.integral_calc_create(net,len(radii_array),radii_array,
                                                           prob_link,analysis_type,
                                                           cont_space,
                                                           set_progressor_callback))

    dll.calc_get_all_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    dll.calc_get_output_length.restype = ctypes.c_int
    outlength = dll.calc_get_output_length(calculation)
    names = dll.calc_get_all_output_names(calculation)
    dll.calc_get_short_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    shortnames = dll.calc_get_short_output_names(calculation)
    sn=[]
    for i in range(outlength):
        sn += [shortnames[i]]

    offset = 0
    net_definition(net)
    offset = 1000
    net_definition(net)
    
    dll.calc_run(calculation)

    dll.net_print(net)
    
    # for debug display we want to invert output arrays
    out_buffer_type = ctypes.c_double * outlength
    out_buffer = out_buffer_type()
    out_array = []
    for i in current_net_arcids:
        dll.calc_get_all_outputs(calculation,out_buffer,i)
        out_array += [list(out_buffer)]

    numlinks = len(current_net_arcids)/2
    for i in range(numlinks):
        print "checking..."
        print current_net_arcids[i],out_array[i]
        print current_net_arcids[i+numlinks],out_array[i+numlinks]
        assert(out_array[i]==out_array[i+numlinks])

    print "Passed duplicate test"

    dll.net_destroy(net)
    dll.calc_destroy(calculation)



    
def test_net_all_options(test_name,net_definition,euclidean_radii):
    print "%s, Angular analysis, discrete space, unweighted"%test_name
    test_net(net_definition,euclidean_radii,ANGULAR,False,False,12)
    print "\n\n%s, Angular analysis, cont space, unweighted"%test_name
    test_net(net_definition,euclidean_radii,ANGULAR,True,False,12)
    print "\n\n%s, Euclidean analysis, discrete space, unweighted"%test_name
    test_net(net_definition,euclidean_radii,EUCLIDEAN,False,False,12)
    print "\n\n%s, Euclidean analysis, cont space, unweighted"%test_name
    test_net(net_definition,euclidean_radii,EUCLIDEAN,True,False,12)

    print "%s, Angular analysis, discrete space, weighted"%test_name
    test_net(net_definition,euclidean_radii,ANGULAR,False,True,12)
    print "\n\n%s, Angular analysis, cont space, weighted"%test_name
    test_net(net_definition,euclidean_radii,ANGULAR,True,True,12)
    print "\n\n%s, Euclidean analysis, discrete space, weighted"%test_name
    test_net(net_definition,euclidean_radii,EUCLIDEAN,False,True,12)
    print "\n\n%s, Euclidean analysis, cont space, weighted"%test_name
    test_net(net_definition,euclidean_radii,EUCLIDEAN,True,True,12)
    print "\n\n"

test_net_all_options("Choice Test",choice_test,choice_test_radii)

print "Junction cost test"
test_net(junction_cost_test,junction_cost_test_radii,EUCLIDEAN,False,False,1)

