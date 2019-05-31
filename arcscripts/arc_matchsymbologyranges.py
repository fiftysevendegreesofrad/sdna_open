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

import arcpy,ctypes,sys
from sdnapy import _dll as sdna_dll

target_layer_names = arcpy.GetParameterAsText(0).split(";")
source_layer_name = arcpy.GetParameterAsText(1)
compute_quantiles = arcpy.GetParameter(2)
value_fieldname = arcpy.GetParameterAsText(3)
num_classes = arcpy.GetParameterAsText(4)

num_classes = int(num_classes)

#check arcmap is 10.1 or more
if float(arcpy.GetInstallInfo()["Version"])<10.1:
    arcpy.AddError("Sorry, this tool requires ArcMap 10.1 or later")
    sys.exit(0)

#get data frame, check unambiguous
doc = arcpy.mapping.MapDocument("CURRENT")
df = arcpy.mapping.ListDataFrames(doc,doc.activeView)
if (len(df)>1):
    arcpy.AddError("Ambiguously named data frames - please rename some")
    sys.exit(0)
df = df[0]

# check target layers are unambiguously specified

def layer_name_to_layer(name):
    layerlist = arcpy.mapping.ListLayers(doc,name,df)
    if len(layerlist)>1:
        arcpy.AddError("Ambiguous layer name: "+name)
        sys.exit(0)
    if len(layerlist)==0:
        arcpy.AddError("No such layer name "+name)
        sys.exit(0)
    return layerlist[0]

target_layers = [layer_name_to_layer(x) for x in target_layer_names]

# if source layer exists, copy symbology
if (source_layer_name != ""):
    source_layer = layer_name_to_layer(source_layer_name)
    for name,layer in zip(target_layer_names,target_layers):
        arcpy.AddMessage("Copying symbology from "+source_layer_name+" to "+name)
        arcpy.mapping.UpdateLayer(df,layer,source_layer,True)
else:
    source_layer = None

if compute_quantiles:
    # check symbologies are compatible with changing class breaks
    symtypes = [tl.symbologyType for tl in target_layers]
    for symtype,name in zip(symtypes,target_layer_names):
        if symtype != "GRADUATED_COLORS" and symtype != "GRADUATED_SYMBOLS":
            arcpy.AddError("Unsupported symbology type on layer: "+name)
            arcpy.AddError("Please set layer symbology to Graduated Colors or Graduated Symbols first")
            sys.exit(0)

    symbologies = [tl.symbology for tl in target_layers]

    # check value_fieldname exists. if not, take from first target layer
    # (which will also match source layer if one was specified)
    if value_fieldname=="":
        value_fieldname = symbologies[0].valueField

    #check all target layers contain value_fieldname
    for name,layer in zip(target_layer_names,target_layers):
        fieldnames = [f.name for f in arcpy.ListFields(layer)]
        if value_fieldname not in fieldnames:
            arcpy.AddError("Layer "+name+" does not contain required field "+value_fieldname)
            arcpy.AddError("Quantiles not recomputed")
            sys.exit(0)

    # read in all table data and compute quantiles
    value_list = []
    for layer,name in zip(target_layers,target_layer_names):
        arcpy.AddMessage("Reading data from layer "+name)
        sc = arcpy.SearchCursor(layer)
        for row in sc:
            value = row.getValue(value_fieldname)
            if value is not None:
                value_list.append(value)
        del row
        del sc

    if len(value_list)==0:
        arcpy.AddError("No data found in field "+value_fieldname)
        sys.exit(0)

    arcpy.AddMessage("Computing quantiles")    
    value_list.sort()
    step = float(len(value_list)-1)/float(num_classes)
    classbreaks = []
    for n in range(num_classes+1):
        floating_index = n*step
        break_value = value_list[int(round(floating_index))]
        classbreaks.append(break_value)
    del value_list # it might be big, don't wait for the garbage collector
        
    arcpy.AddMessage("Applying quantiles")
    for s in symbologies:
        s.valueField=value_fieldname
        s.classBreakValues = classbreaks

arcpy.RefreshActiveView()
arcpy.RefreshTOC()
        

