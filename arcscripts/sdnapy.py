#sDNA software for spatial network analysis 
#Copyright (C) 2011-2019 Cardiff University

#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <https://www.gnu.org/licenses/>.

# This is the python wrapper for the sDNA backend DLL

from sdnaexception import SDNAException

from ctypes import *
import sys,os

PY3 = sys.version_info > (3,)
if PY3:
    bytes_to_str = str
    unicode = str
else:
    def bytes_to_str(b,enc):
        return b

# http://stackoverflow.com/questions/17840144/why-does-setting-ctypes-dll-function-restype-c-void-p-return-long
class my_void_p(c_void_p):
    pass

#print "debug!"
#sys.stdin.readline()

_WARNFUNCTYPE = CFUNCTYPE(c_int, c_char_p)
_PROGFUNCTYPE = CFUNCTYPE(c_int, c_float)
                
__sdna_dll_path = ""
__dll_instance = None
__send_message_callback = None # for global issues only - there are other callbacks in constructor for Calculation()

def set_dll_path(path):
        if path is not "":
                global __sdna_dll_path
                __sdna_dll_path = path

def set_sdnapy_message_callback(callback):
        global __send_message_callback
        __send_message_callback = callback

# singleton pattern
# note while this is a private function it does get used externally
# for license checking, so don't change it
def _dll():
        global __dll_instance
        if __dll_instance == None:
                __initialize_dll()
        return __dll_instance
        
def __initialize_dll():
    global __sdna_dll_path,__dll_instance

    if __sdna_dll_path=="":
        dll_name = r"\\sdna_vs2008.dll"
        
        # what kind of platform are we on?
        if c_size_t==c_ulonglong:
                dll_name = r"\\x64"+dll_name # 64-bit
        else:
                assert(c_size_t==c_ulong) # 32-bit
                
        #first look in same directory
        if PY3:
            file = str(__file__)
        else:
            encoding = sys.getfilesystemencoding()
            file = unicode(__file__,encoding)
        dirname = os.path.dirname(file)
        __sdna_dll_path = dirname+dll_name
        
        #this is to allow use of fresh build
        #if dll doesn't exist, look where it would be if we are running from source tree
        if not os.path.exists(__sdna_dll_path):
            if c_size_t==c_ulonglong:
                    __sdna_dll_path = dirname+r"\..\output\release\x64\sdna_vs2008.dll"
            else:
                    __sdna_dll_path = dirname+r"\..\output\release\sdna_vs2008.dll"

        global __send_message_callback
        if __send_message_callback:    
                __send_message_callback("Loading DLL %s"%__sdna_dll_path)

    # if __sdna_dll_path is non empty,
    # custom dll has been set for debugging
    # (do not print path as that will mess up debug tests)
    
    __dll_instance = windll.LoadLibrary(str(__sdna_dll_path))

class Link:
        def __init__(self,fid,points,data):
                self.id = fid
                self.points = points
                self.data = data

class Net:
    '''how the dll inputs network with attached data'''
    def __init__(self):
        self.dll = _dll()
        self.dll.net_create.restype = my_void_p
        self.net = self.dll.net_create()
        if not self.net:
            raise SDNAException("Bad net config")
        self.dll.net_add_polyline_data.restype = c_int
        self.dll.net_add_polyline_text_data.restype = c_int
        self.dll.net_add_polyline_3d.restype = c_int
        
    def __del__(self):
        self.dll.net_destroy(self.net)

    def output(self):
        self.dll.net_print(self.net)

    def reserve(self,num_items):
            self.dll.net_reserve.restype = c_int
            retval = self.dll.net_reserve(self.net,num_items)
            if retval==0:
                    raise SDNAException("Memory error")

    def add_polyline(self,fid,points):
            point_array_x = (c_double*len(points))()
            point_array_y = (c_double*len(points))()
            point_array_z = (c_float*len(points))()
            for i,p in enumerate(points):
                point_array_x[i] = p[0]
                point_array_y[i] = p[1]
                if len(p)==3:
                        point_array_z[i] = p[2]
                else:
                        point_array_z[i] = 0
            
            retval = self.dll.net_add_polyline_3d(self.net,fid,len(points),point_array_x,point_array_y,point_array_z)
            if (not retval):
                raise SDNAException("Bad geometry or memory error encountered in link %d"%fid)            
                
    def add_polyline_data(self,fid,name,data):
            retval = self.dll.net_add_polyline_data(self.net,fid,c_char_p(name.encode("ascii")),c_float(data))
            if (not retval):
                raise SDNAException("Out of memory error")
    
    def add_polyline_text_data(self,fid,name,data):
            retval = self.dll.net_add_polyline_text_data(self.net,fid,c_char_p(name.encode("ascii")),c_char_p(data.encode("ascii")))
            if (not retval):
                raise SDNAException("Out of memory error")
                
    def toString(self,data="",textdata=""):
            def warn(x):
                    #print (x) #uncomment to debug
                    return 0
            # use dll to build net
            c = Calculation("sdnaprepare",("null;data_absolute=%s;data_text=%s"%(data,textdata)),self,_PROGFUNCTYPE(warn),_WARNFUNCTYPE(warn))
            c.run()
            return list(c.get_geom_outputs())[0].toString()

class GeometryItem(object):
        def __init__(self,data):
                self.data = data

def _geom_format(geom):
        '''Converts nested lists of floats to rounded string'''
        try:
                return "("+",".join(map(_geom_format,geom))+")"
        except TypeError: # it wasn't iterable
                return "%.07g"%geom

class GeometryLayer(object): 
        '''how the dll outputs all data'''
        def __init__(self,geometrycollection):
                self.gc = geometrycollection
                self.dll = _dll()
                self.dll.geom_get_name.restype = c_char_p
                self.name = bytes_to_str(self.dll.geom_get_name(self.gc),"ascii")
                self.dll.geom_get_type.restype = c_char_p
                self.type = bytes_to_str(self.dll.geom_get_type(self.gc),"ascii")
                self.dll.geom_get_field_metadata.restype = c_long
                c_datanames = POINTER(c_char_p)()
                c_shortdatanames = POINTER(c_char_p)()
                c_datatypes = POINTER(c_char_p)()
                self.datalength = self.dll.geom_get_field_metadata(self.gc,byref(c_datanames),byref(c_shortdatanames),byref(c_datatypes))
                self.datanames = [bytes_to_str(x,"ascii") for x in c_datanames[0:self.datalength]]
                self.shortdatanames = [bytes_to_str(x,"ascii") for x in c_shortdatanames[0:self.datalength]]
                self.datatypes = [bytes_to_str(x,"ascii") for x in c_datatypes[0:self.datalength]]
                
        def toString(self):
                output = "%s - %s (%d items)\n"%(self.name,self.type,self.get_num_items())
                output += list(zip(self.datanames,self.shortdatanames,self.datatypes)).__repr__()
                for item in self.get_items():
                        output += "\n"
                        output += list(zip(self.shortdatanames,item.data)).__repr__()
                        output += "  " + _geom_format(item.geom)
                return output

        def get_num_items(self):
                self.dll.geom_get_num_items.restype = c_long
                return self.dll.geom_get_num_items(self.gc)

        def get_items(self):
                class SDNAOutputUnion(Union):
                    _fields_ = [("FLOAT",c_float),("STR",c_char_p),("INT",c_long)]
                    
                self.dll.geom_iterator_create.restype = my_void_p
                it = self.dll.geom_iterator_create(self.gc)
                self.dll.geom_iterator_next.restype = c_int
                self.dll.geom_iterator_getpart.restype = c_long
                num_parts = c_long()
                data = POINTER(SDNAOutputUnion)()
                point_array_x = POINTER(c_double)()
                point_array_y = POINTER(c_double)()
                point_array_z = POINTER(c_float)()
                count = 0
                while self.dll.geom_iterator_next(it,byref(num_parts),byref(data))==1:
                        datalist = [getattr(d,t) for d,t in zip(data[0:self.datalength],self.datatypes)]
                        datalist = [bytes_to_str(x,"ascii") if type(x)==bytes else x for x in datalist]
                        item = GeometryItem(datalist)
                        geom = []
                        count += 1
                        for i in range(num_parts.value):
                                num_points = self.dll.geom_iterator_getpart(it,c_long(i),
                                                                            byref(point_array_x),
                                                                            byref(point_array_y),
                                                                            byref(point_array_z))
                                points = list(zip(list(point_array_x[0:num_points]),
                                             list(point_array_y[0:num_points]),
                                             list(point_array_z[0:num_points])))
                                geom.append(points)
                        item.geom = geom
                        yield item
                self.dll.geom_iterator_destroy(it)

class TableCollection(object):
    '''how the dll inputs table data'''
    def __init__(self):
        self.dll=_dll()
        self.dll.table_collection_create.restype = my_void_p
        self.tc = self.dll.table_collection_create()
    def add_table(self,table):
        self.dll.table_collection_add_table(self.tc,table.table)
                
class Calculation(object):
        def __init__(self,name,config_string,net,set_progressor_callback,set_warning_callback,tablecollection1d=None):
                # default argument can't call TableCollection() as dll won't be initialized yet
                if tablecollection1d==None:
                    tablecollection1d=TableCollection()
                    
                self.dll = _dll()
                # warnfunc must stay in scope as it is a callback
                # hence it is made into a class member
                if type(set_warning_callback)==_WARNFUNCTYPE:
                    # support callback using python 2 string types for legacy test suite
                    self.warnfunc = set_warning_callback
                else:
                    self.warnfunc = _WARNFUNCTYPE(lambda x: set_warning_callback(bytes_to_str(x,"ascii")))

                # progfunc must stay in scope as it is a callback
                # hence it is made into a class member
                self.progfunc = _PROGFUNCTYPE(set_progressor_callback)

                self.dll.calc_create.restype = my_void_p
                self.calc = self.dll.calc_create(c_char_p(name.encode("ascii")),
                                                          c_char_p(config_string.encode("ascii")),
                                                          net.net,
                                                          self.progfunc,
                                                          self.warnfunc,
                                                          tablecollection1d.tc)
                if not self.calc:
                        raise SDNAException('Bad config')
                
        def run(self):
            self.dll.calc_run.restype = c_int
            return self.dll.calc_run(self.calc)==1

        def expected_data_net_only(self):
                self.dll.calc_expected_data_net_only.restype = c_long
                expected_names = POINTER(c_char_p)()
                num_names = self.dll.calc_expected_data_net_only(self.calc,byref(expected_names))
                return [bytes_to_str(x,"ascii") for x in expected_names[0:num_names]]

        def expected_text_data(self):
                self.dll.calc_expected_text_data.restype = c_long
                expected_names = POINTER(c_char_p)()
                num_names = self.dll.calc_expected_text_data(self.calc,byref(expected_names))
                return [bytes_to_str(x,"ascii") for x in expected_names[0:num_names]]
                
        def get_geom_outputs(self):
                self.dll.calc_get_num_geometry_outputs.restype = c_long
                n = self.dll.calc_get_num_geometry_outputs(self.calc)
                self.dll.calc_get_geometry_output.restype = my_void_p
                for i in range(n):
                        yield GeometryLayer(self.dll.calc_get_geometry_output(self.calc,c_long(i)))

        def toString(self):
                return "\n".join([gl.toString() for gl in self.get_geom_outputs()])
        
        def add_table2(self,table):
            self.dll.calc_add_table2d(self.calc,table.table)
        
        def __del__(self):
                self.dll.calc_destroy(self.calc)

def table_result_to_exception(result):
    if result==0:
        return
    elif result==2:
        raise SDNAException("Out of memory")
    elif result==1:
        raise SDNAException("Duplicate table entries")
    else:
        raise SDNAException("Unknown table error")
                
class Table(object):
    def __init__(self,name,zonefieldname):
        self.zonefieldname = zonefieldname
        self.name=name
        self.dll=_dll()
        self.dll.table_create.restype = my_void_p
        self.table = self.dll.table_create(c_char_p(self.name.encode("ascii")),c_char_p(zonefieldname.encode("ascii")))
    def addrow(self,zone,data):
        result = self.dll.table_addrow(self.table,c_char_p(zone.encode("ascii")),c_float(data))
        table_result_to_exception(result)
        
class Table2d(object):
    def __init__(self,name,zonefieldnames):
        origfieldname,destfieldname = zonefieldnames
        self.name=name
        self.dll=_dll()
        self.dll.table2d_create.restype = my_void_p
        self.table = self.dll.table2d_create(c_char_p(self.name.encode("ascii")),c_char_p(origfieldname.encode("ascii")),c_char_p(destfieldname.encode("ascii")))
    def addrow(self,z1,z2,data):
        result = self.dll.table2d_addrow(self.table,c_char_p(z1.encode("ascii")),c_char_p(z2.encode("ascii")),c_float(data))
        table_result_to_exception(result)
        