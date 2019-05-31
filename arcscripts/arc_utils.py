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

import arcpy
from arcpy import env
import math

'''handy utils for arcpy'''

def get_tolerance(in_polyline_feature_class):
    if env.XYTolerance != None:
        tolerance = env.XYTolerance
        arcpy.AddMessage("Using XYTolerance specified in environment: %f"%tolerance)
        return tolerance
    else:
       try:
            tolerance = arcpy.Describe(in_polyline_feature_class).spatialReference.XYTolerance
            arcpy.AddMessage("Using XYTolerance of input feature class: %f"%tolerance)
            return tolerance
       except:
            arcpy.AddError("Unable to obtain XYTolerance for near miss connection check")
            arcpy.AddError("  Either add a spatial reference to the feature class,")
            arcpy.AddError("  or override XYTolerance in the script environment settings.")
            raise 

def get_z_tolerance(in_polyline_feature_class):
    if env.ZTolerance != None:
        tolerance = env.ZTolerance
        arcpy.AddMessage("Using ZTolerance specified in environment: %f"%tolerance)
        return tolerance
    else:
       try:
            tolerance = arcpy.Describe(in_polyline_feature_class).spatialReference.ZTolerance
            if not math.isnan(tolerance):
                arcpy.AddMessage("Using ZTolerance of input feature class: %f"%tolerance)
                return tolerance
            else:
                arcpy.AddMessage("Input feature class has spatial reference but no z tolerance")
                arcpy.AddMessage("Using z tolerance of 0")
                return 0
       except:
            arcpy.AddError("Unable to obtain ZTolerance for near miss connection check")
            arcpy.AddError("  Either add a spatial reference to the feature class,")
            arcpy.AddError("  or override ZTolerance in the script environment settings.")
            raise 

def clear_selection(layer):
    arcpy.SelectLayerByAttribute_management(layer, "CLEAR_SELECTION")

def select_edges(layer,in_arc_idfield,edge_list):
    # build where clause
    sql_clause = ' OR '.join(['"%s"=%s'%(in_arc_idfield,str(link)) for link in edge_list])
    arcpy.SelectLayerByAttribute_management(layer, "NEW_SELECTION", sql_clause)
    arcpy.RefreshActiveView()
