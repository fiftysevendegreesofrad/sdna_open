import arcpy
import os,time,sys
import ctypes

outlength = None
names = None

def add_dll_defined_fields_to_table(dll,calculation,table,overwrite):
    global outlength, names
    
    arcpy.AddMessage('  Overwrite output = %s'%overwrite)
    
    # get output names    
    dll.calc_get_all_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    dll.calc_get_short_output_names.restype = ctypes.POINTER(ctypes.c_char_p)
    dll.calc_get_output_length.restype = ctypes.c_int

    outlength = dll.calc_get_output_length(calculation)
    alii = list(dll.calc_get_all_output_names(calculation)[0:outlength]);
    names = list(dll.calc_get_short_output_names(calculation)[0:outlength]);

    # ensure names are valid for table type
    names = [arcpy.ValidateFieldName(x,os.path.dirname(table)) for x in names]

    # check fields won't be overwritten unless specified
    existing_field_names = [x.name for x in arcpy.ListFields(table)]
    if not overwrite:
        error_happened = False
        for name,alias in zip(names,alii):
            if name in existing_field_names:
                    arcpy.AddError('Field %s (%s) exists already'%(name,alias))
                    error_happened = True
        if error_happened:
            arcpy.AddError("Either enable 'Overwrite output fields' in the tool dialog box\n\
                            Or delete/rename the existing fields")
            raise Exception("Can't overwrite output data")

    arcpy.SetProgressor("step", "Checking output columns", 0, outlength, 1)
            
    # create fields if needed
    for i,(name,alias) in enumerate(zip(names,alii)):
        arcpy.SetProgressorPosition(i)
        if name not in [x.name for x in arcpy.ListFields(table)]:
            arcpy.AddMessage('    Field %s (%s) not present, adding'%(name,alias))
            arcpy.AddField_management(table,name,'FLOAT',field_alias=alias)
        else:
            arcpy.AddMessage('    Field %s (%s) exists already, overwriting'%(name,alias))

    arcpy.SetProgressorPosition(outlength)

def populate_dll_defined_fields(dll,calculation,table,idfield):
    global outlength,names
    out_buffer_type = ctypes.c_float * outlength
    out_buffer = out_buffer_type()

    num_rows = int(arcpy.GetCount_management(table).getOutput(0))
    if num_rows > 100:
        increment = num_rows/100
    else:
        increment = num_rows
        
    arcpy.SetProgressor("step", "Writing output", 0, num_rows, increment)

    rows = arcpy.UpdateCursor(table)
    for n,row in enumerate(rows):
        if n%increment==0:
            arcpy.SetProgressorPosition(n)
        arcid = row.getValue(idfield)
        dll.calc_get_all_outputs(calculation,out_buffer,arcid)
        for i,name in enumerate(names):
            row.setValue(name,out_buffer[i])
        rows.updateRow(row)

    arcpy.SetProgressorPosition(num_rows)

    del row
    del rows

CALLBACKFUNCTYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_long)
def set_progressor(x):
    print( "progress callback: %d" % x)
    return 0
set_progressor_callback = CALLBACKFUNCTYPE(set_progressor)
WARNINGCALLBACKFUNCTYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_char_p)
def warning(x):
    print("warning callback: %s" % x)
    return 0
warning_callback = WARNINGCALLBACKFUNCTYPE(warning)

last_time = 0
def timestamp():
    global last_time
    result = time.clock() - last_time
    # arcpy.AddMessage("took %f ms"%(result*1000))
    last_time = time.clock()

def string_to_radius(r):
    if r=='n' or r=='N':
        return -1
    else:
        try:
            result = float(r)
        except ValueError:
            errorstring = 'Invalid radius specified: %s'%r
            arcpy.AddError(errorstring)
            raise Exception(errorstring)
        return result

def string_to_measure_index(s):
    return ctypes.c_int(['ANGULAR','EUCLIDEAN'].index(s))

def integral_common(in_polyline_feature_class,in_start__gsation,in_end__gsation,in_output_measure,continuous_space,in_radii,
                in_problem_link,overwrite_output,weight_activity_by_link_length,custom_activity_weight_field=""):

    assert (in_problem_link==-1)
    global last_time
    last_time = time.clock()

    if continuous_space:
        space = "continuous"
    else:
        space = "discrete"
    if custom_activity_weight_field:
        custom = "custom"
    else:
        custom = "constant"
    if weight_activity_by_link_length:
        weighting = "length"
    else:
        weighting = ""
    arcpy.AddMessage("%s analysis, %s space, %s %s weighting"%(in_output_measure,space,custom,weighting))

    if not arcpy.Describe(in_polyline_feature_class).hasOID:
        msg = 'Feature class has no object ID field'
        arcpy.AddError(msg)
        raise AttributeError(msg)
    in_arc_idfield = arcpy.Describe(in_polyline_feature_class).OIDfieldname

    shapefieldname = arcpy.Describe(in_polyline_feature_class).ShapeFieldName

    fieldnames = [f.name for f in arcpy.ListFields(in_polyline_feature_class)]
    if in_start__gsation != "" or in_end__gsation != "":
        if not in_start__gsation in fieldnames:
            msg = 'Start _gsation field does not exist: %s' % in_start__gsation
            arcpy.AddError(msg)
            raise AttributeError(msg)
        if not in_end__gsation in fieldnames:
            msg = 'End _gsation field does not exist: %s'%in_end__gsation
            arcpy.AddError(msg)
            raise AttributeError(msg)
        using__gsation = True
    else:
        using__gsation = False

    if custom_activity_weight_field != "":
        if not custom_activity_weight_field in fieldnames:
            msg = 'Custom activity weight field does not exist: %s'
            msg %= custom_activity_weight_field
            arcpy.AddError(msg)
            raise AttributeError(msg)
        using_custom_weight = True
    else:
        using_custom_weight = False

    num_rows = int(arcpy.GetCount_management(in_polyline_feature_class).getOutput(0))
    if num_rows == 0:
        msg = 'No rows in input'
        arcpy.AddError(msg)
        raise AttributeError(msg)
    arcpy.AddMessage('Input has %d rows'%num_rows)

    radii = map(string_to_radius,in_radii)
    output_measure = string_to_measure_index(in_output_measure)

    radii_array = (ctypes.c_double*len(radii))()
    for i,r in enumerate(radii):
            radii_array[i] = r

    #initialize dll and create add_polyline wrapper func
    dirname = os.path.dirname(sys.argv[0])
    dllpath = dirname+r"\\..\\Release\\sdna_vs2008.dll"
    print(dllpath)
    dll = ctypes.windll.LoadLibrary(dllpath)

    net = ctypes.c_void_p(dll.net_create(weight_activity_by_link_length))

    calculation = ctypes.c_void_p(dll.integral_calc_create(net,len(radii_array),radii_array,in_problem_link,
                                                           output_measure,continuous_space,
                                                           set_progressor_callback,
                                                           warning_callback))

    def add_polyline(arcid,points,_gs,weight):
        point_array_x = (ctypes.c_double*len(points))()
        point_array_y = (ctypes.c_double*len(points))()
        for i,(x,y) in enumerate(points):
            point_array_x[i] = x
            point_array_y[i] = y
        dll.net_add_polyline(net,arcid,len(points),point_array_x,point_array_y)
        dll.net_add_polyline_data(net,arcid,"start_gs",ctypes.c_float(_gs[0]))
        dll.net_add_polyline_data(net,arcid,"end_gs",ctypes.c_float(_gs[1]))
        dll.net_add_polyline_data(net,arcid,"weight",ctypes.c_float(weight))
        dll.net_add_polyline_data(net,arcid,"custom_cost",ctypes.c_float(1))
        dll.net_add_polyline_data(net,arcid,"is_island",ctypes.c_float(0))

    timestamp()

    arcpy.AddMessage('Creating output fields...')
    add_dll_defined_fields_to_table(dll,calculation,in_polyline_feature_class,overwrite_output)

    timestamp()

    if num_rows > 100:
        increment = num_rows/100
    else:
        increment = num_rows
        
    arcpy.SetProgressor("step", "sDNA reading feature class from ARC", 0, num_rows, 1)

    #read feature class and send to dll
    curr_row = 0
    rows = arcpy.SearchCursor(in_polyline_feature_class)
    for row in rows:
        if curr_row%increment == 0:
            arcpy.SetProgressorPosition(curr_row)
        curr_row += 1
        
        shape = row.getValue(shapefieldname)
        arcid = row.getValue(in_arc_idfield)

        if using__gsation:
            start_gs = row.getValue(in_start__gsation)
            end_gs = row.getValue(in_end__gsation)
        else:
            start_gs = 0
            end_gs = 0

        if using_custom_weight:
            weight = row.getValue(custom_activity_weight_field)
        else:
            weight = 1

        # read geometry
        pointlist = []
        for i in range(shape.partCount):
            for point in shape.getPart(i):
                pointlist.append((point.X,point.Y))

        add_polyline(arcid,pointlist,(start_gs,end_gs),weight)

    arcpy.SetProgressorPosition(curr_row)

    timestamp()

    arcpy.AddMessage('Running...')

    arcpy.SetProgressor("step", "sDNA processing", 0, num_rows, 1)
    arcpy.SetProgressorPosition(0)
    dll.calc_run(calculation)
    arcpy.SetProgressorPosition(num_rows)

    timestamp()

    arcpy.AddMessage('Copying output back to table...')

    populate_dll_defined_fields(dll,calculation,in_polyline_feature_class,in_arc_idfield)

    timestamp()

    dll.net_destroy(net)
    dll.calc_destroy(calculation)

for i in range(10):
    print('%s: %s' % i,arcpy.GetParameterAsText(i))

#get input params
in_polyline_feature_class = arcpy.GetParameterAsText(0)
in_start__gsation = arcpy.GetParameterAsText(1)
in_end__gsation = arcpy.GetParameterAsText(2)
in_output_measure = arcpy.GetParameterAsText(3)
continuous_space = bool(arcpy.GetParameter(4))
in_radii = arcpy.GetParameterAsText(5).split(',')
overwrite_output = bool(arcpy.GetParameter(6))
weight_activity_by_link_length = bool(arcpy.GetParameter(7))
custom_activity_weight_field = arcpy.GetParameterAsText(8)

# the empty string 7th argument is necessary to prevent use of problem route code
integral_common(in_polyline_feature_class,in_start__gsation,in_end__gsation,in_output_measure,continuous_space,in_radii,
                -1,overwrite_output,weight_activity_by_link_length,custom_activity_weight_field)
