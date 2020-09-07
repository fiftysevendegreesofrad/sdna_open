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

import sys
import re
import os
from sdnapy import *
import time
import gc

#sys.stdin.readline()

'''
This is the common logic surrounding any sdna calculation
It plugs a data handling environment (e.g. arcgis, shapefiles/command line)
into the C++ backend
providing useful error messages along the way

To developers:  This file also serves as an example of how to use sdnapy.
It might help you to know that env is a facade for reading and writing data
and user messages
(i.e. this code is designed to run in different environments)
'''

num_progress_steps = 1000

def runcalculation(env, #environment object, IO etc
                   name, #name of sdna calculation to run
                    config_string, #config as specified in sDNA Advanced Config document
                    input_map, #dictionary mapping input names (such as "net") to their handles ("c:\myfolder\inputnet")
                    output_map, #dictionary mapping output names (such as "net") to their handles ("c:\myfolder\outputnet")
                                #The output names created depend on config_string - see use of get_geom_outputs() below
                    dll=""): #option to specify different dll for debugging - not relevant to you

    env.AddMessage("sDNA %s config: %s"%(str(name,"utf-8"),str(config_string,"utf-8")))
    set_dll_path(dll)
    
    # if we ever start having multiple geometry inputs we need to address the input spatial referencing bug
    geominputs = [x for x in input_map if x != b"tables"]
    assert len(geominputs)==1

    def set_progressor_callback(x):
        env.SetProgressorPosition(int(x/100*num_progress_steps))
        return 0

    def warning_callback(x):
        x = str(x,"utf-8")
        if re.match("WARNING:",x):
            env.AddWarning(" %s"%x)
        elif re.match("ERROR:",x):
            env.AddError(" %s"%x)
        else:
            env.AddMessage("  %s"%x)
        return 0

    set_sdnapy_message_callback(env.AddMessage)
    input_handle = input_map[b"net"];
    
    input_fieldnames = env.ListFields(input_handle)

    #read tables and send to dll at this stage (as tables inform expected zone fields calc wants to see on network)
    tablecollection1d = TableCollection()
    table2d = None
    
    tablenames = []
    if b"tables" in input_map:
        for tablesource in input_map[b"tables"].split(","):
            tablesource = tablesource.strip()
            if tablesource!="":
                env.AddMessage("Reading table file "+tablesource)
                for envtable in env.ReadTableFile(tablesource):
                    if envtable.dims==1:
                        env.AddMessage(" ...1d variable "+envtable.name)
                        tablenames += [envtable.name]
                        table = Table(envtable.name,envtable.zonefieldnames[0])
                        for zone,data in envtable.rows():
                            table.addrow(zone[0],data)
                        tablecollection1d.add_table(table)
                    else:
                        assert envtable.dims==2
                        # sdna only supports a single 2d table which gets used as OD matrix
                        if (table2d):
                            multiple2dtables = [table2d.name,envtable.name]
                            env.AddMessage("Error: both %s and %s are 2d tables; only one is allowed"%multiple2dtables)
                            sys.exit(1)
                        env.AddMessage(" ...2d variable "+envtable.name)
                        table = Table2d(envtable.name,envtable.zonefieldnames)
                        for (z1,z2),data in envtable.rows():
                            table.addrow(z1,z2,data)
                        table2d = table
                    del envtable

    net = Net()
    calculation = Calculation(name,config_string,net,set_progressor_callback,warning_callback,tablecollection1d)
    if table2d:
        calculation.add_table2(table2d)
    
    allfieldnames = env.ListFields(input_handle)
    for fieldname in allfieldnames:
        if fieldname in tablenames:
            env.AddError("Field %s appears both on network and in tables, ambiguous"%fieldname)
            sys.exit(1)
    
    allfieldtypes = env.ListFieldTypes(input_handle)
    numericfields = [f for f,t in zip(allfieldnames,allfieldtypes) if t==int or t==float]
    textfields = [f for f,t in zip(allfieldnames,allfieldtypes) if t==str]
    names_to_load = [str(x,"utf-8") for x in calculation.expected_data_net_only()]
    text_names_to_load = [str(x,"utf-8") for x in calculation.expected_text_data()]
    for name in names_to_load:
        if name not in numericfields:
            env.AddError("Calculation expected numeric field '%s' on network; not found"%name)
            sys.exit(1)
    for name in text_names_to_load:
        if name not in textfields:
            env.AddError("Calculation expected text field '%s'; not found in data"%name)
            sys.exit(1)
    
    num_rows = env.GetCount(input_handle)
    net.reserve(num_rows)
    
    if num_rows == 0:
        env.AddError('No rows in input')
        sys.exit(1)
    env.AddMessage('Input has %d rows'%num_rows)
    
    # ensure we can create output geometries
    chunks = 1 # set higher to split output into multiple chunks e.g. if shapefile output is too large for memory
    geom_create_cursors = []
    for geom in calculation.get_geom_outputs():
        chunks_this_geom = []
        for c in range(chunks):
            outname = output_map[geom.name]
            if chunks>1:
                outname += "_c%d"%c
            env.AddMessage("Creating geometry output "+outname)
            chunks_this_geom += [env.GetCreateCursor(outname,geom.shortdatanames,geom.datanames,geom.datatypes,geom.type)]
        geom_create_cursors += [(geom,chunks_this_geom)]

    #read feature class and send to dll
    for fid,points,fielddata in env.ReadFeatures(input_handle,names_to_load+text_names_to_load):
        net.add_polyline(fid,points)
        for n,value in zip(names_to_load,fielddata[0:len(names_to_load)]):
            if value is None:
                env.AddError("Null value encountered in field %s"%n)
                sys.exit(1)
            net.add_polyline_data(fid,n.encode("utf-8"),value)
        for n,value in zip(text_names_to_load,fielddata[len(names_to_load):]):
            if value is None:
                env.AddError("Null value encountered in field %s"%n)
                sys.exit(1)
            net.add_polyline_text_data(fid,n.encode("utf-8"),value)
            
    env.SetProgressor("step", "sDNA processing", num_progress_steps)
    env.SetProgressorPosition(0)
    st = time.clock()
    result = calculation.run()
    et = time.clock()
    env.SetProgressorPosition(num_progress_steps)
    if dll=="":
        env.AddMessage('TIME %.1f for %s'%(et-st,config_string))

    if result:
        for geom,chunks in geom_create_cursors:     
            items = geom.get_num_items()
            items_per_chunk = items/len(chunks)
            if items%len(chunks)!=0:
                items_per_chunk+=1
            
            cc = chunks[0]
            chunks = chunks[1:]
            cc.StartProgressor(items_per_chunk)
            items_this_chunk = 0
            
            for item in geom.get_items():
                if items_this_chunk >= items_per_chunk:
                    cc.Close()
                    cc = chunks[0]
                    chunks = chunks[1:]
                    cc.StartProgressor(items_per_chunk)
                    items_this_chunk = 0
                    
                cc.AddRowGeomItem(item)
                items_this_chunk+=1
            cc.Close()

    del net
    del calculation
    
    if not result:
        env.AddError("Calculation failed.")
        sys.exit(1)
        
