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
import os

def add_field_to_table(table,idfield,name,datatype,arcid_to_data_function):
    arcpy.AddMessage('adding field')
    arcpy.AddField_management(table,name,datatype)
    arcpy.AddMessage('added field... populating now...')

    rows = arcpy.UpdateCursor(table)
    for row in rows:
        row.setValue(name,arcid_to_data_function(row.getValue(idfield)))
        rows.updateRow(row) 
    del row
    del rows
    arcpy.AddMessage('...done')

def add_multiple_fields_to_table(table,idfield,names,alii,datatype,arcid_to_data_function):
    names = [arcpy.ValidateFieldName(x,os.path.dirname(table)) for x in names]
    arcpy.AddMessage('adding fields: '+','.join(names))
    for name,alias in zip(names,alii):
        if name not in [x.name for x in arcpy.ListFields(table)]:
            arcpy.AddField_management(table,name,datatype,field_alias=alias)
        else:
            arcpy.AddWarning('field %s exists already, overwriting'%name)
    arcpy.AddMessage('added fields... populating now...')

    rows = arcpy.UpdateCursor(table)
    for row in rows:
        values = arcid_to_data_function(row.getValue(idfield))
        for i,name in enumerate(names):
            row.setValue(name,values[i])
        rows.updateRow(row) 
    del row
    del rows
    arcpy.AddMessage('...done')
