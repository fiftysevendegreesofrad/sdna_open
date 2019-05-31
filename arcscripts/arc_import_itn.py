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

import arcpy, os

in_file = arcpy.GetParameterAsText(0)
out_folder = arcpy.GetParameterAsText(1)
out_name = arcpy.GetParameterAsText(2)

#check output name is valid
parts = out_name.split(".")
base = parts[0]

if len(parts)>2 or (len(parts)==2 and parts[1].lower() != "gdb"):
    raise StandardError, "Invalid gdb name"

#name constants
out_gdb = out_folder+"/"+base+".gdb"
out_fc = out_gdb+"/%s_sDNA"%base

itn_startelev_field = "DirectedNode_NegativeOrientation_GradeSeparation"
itn_endelev_field = "DirectedNode_PositiveOrientation_GradeSeparation"

#check output gdb doesn't exist already
if os.path.exists(out_gdb):
    raise StandardError, "Output gdb already exists"

#then the actual work
arcpy.AddMessage("Importing now - this may take a long time, especially if road routing information is included")

arcpy.AddMessage("Calling Data Interoperability import...")
arcpy.AddMessage("(If this step fails, check you have ArcGIS Data Interoperability extension installed.  A license for the extension is NOT needed.")
arcpy.QuickImport_interop("GMLSF,"+in_file, out_gdb)
arcpy.AddMessage("Data Interop import complete, converting RoadLink layer to sDNA format...")

arcpy.CopyFeatures_management(out_gdb+"/RoadLink",out_fc)

arcpy.AddField_management(out_fc,"startelev","SHORT")
arcpy.AddField_management(out_fc,"endelev","SHORT")
arcpy.AddField_management(out_fc,"island","SHORT")

arcpy.CalculateField_management(out_fc,"startelev","!%s!"%itn_startelev_field,"PYTHON")
arcpy.CalculateField_management(out_fc,"endelev","!%s!"%itn_endelev_field,"PYTHON")
arcpy.CalculateField_management(out_fc,"island",'int(!NatureOfRoad!=="Traffic Island Link" or !NatureOfRoad!=="Traffic Island Link At Junction")',"PYTHON")

arcpy.DeleteField_management(out_fc,itn_startelev_field)
arcpy.DeleteField_management(out_fc,itn_endelev_field)

arcpy.AddMessage("Successfully imported to %s"%out_gdb)

#add result to display (remarkably faffy...)
arcpy.AddMessage("Adding to display...")
doc = arcpy.mapping.MapDocument("CURRENT")
df = arcpy.mapping.ListDataFrames(doc,doc.activeView)
if (len(df)>1):
    arcpy.AddWarning("Ambiguously named data frames in your document - hope I added the results to the right one!  Check the others if need be.")
newlayer = arcpy.mapping.Layer(out_fc)  
arcpy.mapping.AddLayer(df[0],newlayer)
arcpy.RefreshActiveView()
arcpy.RefreshTOC()
del doc,df,newlayer
