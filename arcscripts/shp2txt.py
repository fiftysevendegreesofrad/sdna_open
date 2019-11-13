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

import sys,glob,shapefile

filenames = sys.argv[1:]

def prettify(stuff,precision):
    if type(stuff)==list:
        raise "Crap"
    try:
        formatstring = "%%.%df"%precision
        return formatstring%float(stuff)
    except TypeError:
        return str(stuff)
    except ValueError:
        return str(stuff)

def structure(x,depth=-1):
    depth += 1
    prefix = " "*depth
    if not hasattr(x,"__iter__"):
        return prefix+prettify(x,20)+"\n"
    else:
        if all([not hasattr(i,"__iter__") for i in x]):
            return prefix+",".join([prettify(i,20) for i in x])+"\n"
        else:
            retval=prefix+"[\n"
            for i in x:
                retval += structure(i,depth)
            retval+=prefix+"]\n"
            return retval

def _sf_getfields(filename):
    sf = shapefile.Reader(filename)
    fields = sf.fields[1:] # ignore deletion flag
    fieldnames = [x[0] for x in fields]
    return fieldnames

def makenestedlist(shape):
    points = shape.points
    if hasattr(shape,"z"):
                points = [(x,y,z) for (x,y),z in zip(points,shape.z)]
    parts = list(shape.parts) + [len(points)]
    return [points[parts[x]:parts[x+1]] for x in range(len(parts)-1)]

def ReadFeatures(filename):
    sf = shapefile.Reader(filename)
    data = sf.shapeRecords()
    fieldnames = _sf_getfields(filename)
    fid = 0
    for record in data:
        attrdict = {}
        for name,value in zip(fieldnames,record.record):
            attrdict[name]=prettify(value,20)
        yield fid,makenestedlist(record.shape),attrdict
        fid += 1

for pattern in filenames:
    for filename in glob.glob(pattern+".shp"):
        print "Shapefile ",filename,"===================================="
        
        for fid,shapes,attr in ReadFeatures(filename):
            print "Item",fid
            print structure(shapes),
            print attr

        print
        print
        print
