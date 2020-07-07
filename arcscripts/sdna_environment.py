<<<<<<< HEAD
# (c) Crispin Cooper on behalf of Cardiff University 2013-2015
# This file is released under MIT license

# This encapsulates the different environments in which the
# sDNA python scripts might run.
# At present there are two: 1. within ArcGIS; 2. directly on shapefiles

import abc,sys,os,re,shutil,csv
from os import path
from collections import defaultdict

def is_gdb(name):
    # anything that contains literal .gdb followed by / or \
    return bool(re.match(r".*\.gdb[/\\].*",name.lower()))

def stripshpextensions(name):
    ext = name[-4:].lower()
    if ext in [".shp",".shx",".dbf"]:
        return name[:-4]
    else:
        return name

def get_sensible_increment(numitems):
    inc = numitems/1000 if numitems>100 else numitems
    return inc if inc>0 else 1
        
class SdnaEnvironment:
    __metaclass__ = abc.ABCMeta

    def __init__(self):
        self.tty_unfinished_line = False
        self.progressor_output_100 = False
        self.is_interactive = sys.stdout.isatty()
        
    def SetProgressor(self,progressor_type,message,end):
        assert(progressor_type=="step" or progressor_type=="default")
        self.progressor_end = end
        self.AddMessage(message)
        if self.tty_unfinished_line:
            sys.stderr.write("\n")
            sys.stderr.flush()
        
    def SetProgressorPosition(self,position):
        if self.progressor_end==0:
            percent = 100
        else:
            percent = 100*float(position)/float(self.progressor_end)
        progressor_string = "%.1f%%"%percent
        if progressor_string != "100.0%":
            self.progressor_output_100 = False
            sys.stderr.write("\rProgress: %s"%progressor_string)
            self.tty_unfinished_line = True
        else:
            # only output "100%" once and follow it with a newline
            if not self.progressor_output_100:
                self.progressor_output_100 = True
                sys.stderr.write("\rProgress: 100%\n")
                self.tty_unfinished_line = False
        sys.stderr.flush()

    def ttyensurenewline(self):
        if self.tty_unfinished_line and self.is_interactive:
            sys.stderr.write("\n")
            self.tty_unfinished_line = False

    def AddWarning(self,warning):
        self.ttyensurenewline()
        sys.stdout.write(warning+"\n")

    def AddError(self,error):
        self.ttyensurenewline()
        sys.stdout.write(error+"\n")

    def AddMessage(self,message):
        self.ttyensurenewline()
        sys.stdout.write(message+"\n")

    @abc.abstractmethod
    def get_out_base(self,in_handle):
        return
    
    @abc.abstractmethod
    def ListFields(self,filename):
        return

    @abc.abstractmethod
    def ListFieldTypes(self,filename):
        return

    @abc.abstractmethod
    def GetCount(self,filename):
        return

    @abc.abstractmethod
    def ReadFeatures(self,filename,fields):
        return

    @abc.abstractmethod
    def GetCreateCursor(self,outfilename,fieldnames,long_fieldnames,fieldtypes,geomtype):
        return
        
    def ReadTableFile(self,source):
        class EnvTable(object):
            def __init__(self,name,zonefieldnames,env):
                if name.strip()=="":
                    env.AddError("Table column with no name")
                    sys.exit(1)
                self.name = name
                self.dims = len(zonefieldnames)
                self.zonefieldnames = zonefieldnames
                self.rowdata = []
                self.zones = []
                self.env = env
            def rows(self):
                for z,d in zip(self.zones,self.rowdata):
                    yield z,d
            def add(self,z,d,i):
                for zpart in z:
                    if zpart.strip()=="":
                        env.AddError("Zone with no name")
                self.zones += [z]
                try:
                    self.rowdata += [float(d)]
                except ValueError:
                    self.env.AddError('Bad data for variable "%s" row %d: "%s"'%(self.name,i,d)) 
                    sys.exit(1)
                
        def check_row_length(i,row,header):
            if len(row)<len(header):
                self.AddError("Row %d too short"%i)
                sys.exit(1)
            if len(row)>len(header):
                self.AddError("Row %d too long"%i)
                sys.exit(1)
                
        def csv_rstrip(line):
            while line[-1].strip()=="":
                line = line[:-1]
            return [x.strip() for x in line]
            
        class csv_rstrip_reader(object):
            def __init__(self,csvfile):
                self.reader = csv.reader(csvfile)
            def next(self):
                return csv_rstrip(self.reader.next())
            def __iter__(self):
                return self

        with open(source, 'rb') as csvfile: #https://stackoverflow.com/questions/40310042/python-read-csv-bom-embedded-into-the-first-key
            reader = csv_rstrip_reader(csvfile)
            metadata = reader.next()
            if metadata[0].lower()=="list": 
                if len(metadata)<2:
                    self.AddError("Table format error: number of zone fields not specified")
                    sys.exit(1)
                try:
                    dims = int(metadata[1])
                except ValueError:
                    self.AddError("Table format error: could not read number of zone fields")
                    sys.exit(1)
                if dims not in [1,2]:
                    self.AddError("Table format error: invalid number of zone fields (only 1 or 2 allowed)")
                    sys.exit(1)
                header = reader.next()
                if len(header)<dims+1:
                    self.AddError("Not enough columns")
                    sys.exit(1)
                tables = [EnvTable(name,header[0:dims],self) for name in header[dims:]]
                for i,row in enumerate(reader):
                    check_row_length(i,row,header)
                    zone = row[0:dims]
                    for t,d in zip(tables,row[dims:]):
                        t.add(zone,d,i)
                return tables
                        
            elif metadata[0].lower()=="matrix":
                if len(metadata)!=3:
                    self.AddError("Table format error: matrix metadata wrong length")
                    sys.exit(1)
                header = reader.next()
                table = EnvTable(header[0].lower(),metadata[1:3],self)
                to_zones = header[1:]
                for i,row in enumerate(reader):
                    check_row_length(i,row,header)
                    from_zone = row[0]
                    for to_zone,d in zip(to_zones,row[1:]):
                        table.add((from_zone,to_zone),d,i)
                return [table]
            
            else:
                self.AddError("Unknown table format: %s"%metadata[0])
                sys.exit(1)
            
                
class CreateCursor:
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def AddRowGeomItem(self,geometryitem):
        return

    @abc.abstractmethod
    def StartProgressor(self,numitems):
        return
        
    @abc.abstractmethod
    def Close(self):
        return

class CSVCreateCursor(CreateCursor):
    def __init__(self,outfilename,long_fieldnames,env):
        self.env = env
        
        if outfilename.lower()[-4:]!=".csv":
            outfilename += ".csv"
            
        try:
            self.outfile = open(outfilename,"wb")
        except:
            env.AddError("Error opening output file "+outfilename)
            sys.exit(1)
            
        self.writer = csv.writer(self.outfile)
        self.writer.writerow(long_fieldnames)
    
    def Close(self):
        if hasattr(self,"outfile"):
            self.outfile.close()
        
    def AddRowGeomItem(self,geometryitem):
        def myformat(x):
            if type(x)==float:
                return ("%.6f"%x).rstrip("0")
            else:
                return x
                
        self.writer.writerow(map(myformat,geometryitem.data))
        
    def StartProgressor(self,numitems):
        # no progressor for now as csv files are expected to be short
        return

class SdnaShapefileEnvironment(SdnaEnvironment):
    max_field_length = 10

    def get_out_base(self,outfilename):
        bits = outfilename.split(".")
        if bits[-1].lower()=="shp":
            del bits[-1]
        return ".".join(bits)

    def __init__(self,spatial_reference_source=""):
        super(SdnaShapefileEnvironment, self).__init__()
        import shapefile
        self.shapefile = shapefile
        projfile = stripshpextensions(spatial_reference_source)+".prj"
        if os.path.exists(projfile):
            self.projfile=projfile
        else:
            self.projfile=None
    
    def _sf_getfields(self,filename):
        try:
            sf = self.shapefile.Reader(filename)
        except:
            self.AddError("Can't open shapefile "+filename)
            sys.exit(1)
            
        fields = sf.fields[1:] # ignore deletion flag
        fieldnames = [x[0] for x in fields]
        fieldtypes = [SdnaShapefileEnvironment._sf_field_to_type(x[1]) for x in fields]
        return fieldnames,fieldtypes

    def ListFields(self,filename):
        return self._sf_getfields(filename)[0]

    def ListFieldTypes(self,filename):
        return self._sf_getfields(filename)[1]

    def GetCount(self,filename):
        sf = self.shapefile.Reader(filename)
        return sf.numRecords

    @staticmethod
    def _sf_field_to_type(letter):
        if letter=="F" or letter=="O":
            return float
        if letter=="L":
            return bool
        if letter=="N" or letter=="I" or letter=="+":
            return int
        if letter=="C" or letter=="D" or letter=="M":
            return str
        return str # default to string

    def ReadFeatures(self,filename,fields):
        self.AddMessage("Reading features from %s"%filename)
        sf = self.shapefile.Reader(filename)
        self.SetProgressor("step","Reading features from %s"%filename,sf.numRecords)
        increment = sf.numRecords/100 if sf.numRecords>100 else sf.numRecords
        data = sf.shapeRecords()
        fieldnames,fieldtypes = self._sf_getfields(filename)
        wanted_field_indices = [fieldnames.index(f) for f in fields]
        wanted_field_types = [fieldtypes[i] for i in wanted_field_indices]
        fid = 0
        for record in data:
            if fid%increment == 0:
                self.SetProgressorPosition(fid)
            fielddata=[]
            for i,typ in zip(wanted_field_indices,wanted_field_types):
                value = record.record[i]
                try:
                    converted_value = typ(value)
                except ValueError:
                    if re.match("^\\x00*$", value):
                        #bit weird but interpret uninitialized fields as default
                        #http://www.dbase.com/Knowledgebase/INT/db7_file_fmt.htm
                        converted_value = typ()
                    else:
                        raise TypeError
                except TypeError:
                    name = fieldnames[i]
                    msg = "Failed to convert %s (in field %s line %s) to %s"%tuple(map(str,[value,name,fid,typ]))
                    self.AddError(msg)
                    sys.exit(1)
                fielddata += [converted_value]
            if list(record.shape.parts) != [0]:
                self.AddError("Multipart features are not supported in sDNA: convert your input data to single part features then rerun")
                sys.exit(1)
            points = record.shape.points
            if hasattr(record.shape,"z"):
                points = [(x,y,z) for (x,y),z in zip(points,record.shape.z)]
            yield fid,points,fielddata
            fid += 1
        self.SetProgressorPosition(fid)

    @classmethod
    def _spot_conflicting_name_stems(cls,fieldnames):
        name_count = defaultdict(int)
        for f in fieldnames:
            name_count[f] += 1
        matching_stems = [cls._name_stem(name) for name,count in name_count.items() if count>1]
        # of course these may themselves contain duplicates - remove them
        matching_stems = dict([(x,None) for x in matching_stems]).keys()
        return matching_stems

    def _dbf_validate_fieldnames(self,fieldnames):
        # truncate, then check for conflicts
        initial_fieldnames = fieldnames
        fieldnames = [x[0:self.max_field_length] for x in fieldnames]
        while True:
            conflicts = self._spot_conflicting_name_stems(fieldnames)
            if not conflicts:
                break
            # replace conflicting names
            #conflicts are 1. any matching names, 2. any names whose stem matches the stem of other conflicts
            for stemname in conflicts:
                must_replace = [self._name_stem(x)==stemname for x in fieldnames]
                count = len([x for x in must_replace if x])
                newnames = self._fix_name(stemname,count)
                for i,(name,must_replace) in enumerate(zip(fieldnames,must_replace)):
                    if must_replace:
                        fieldnames[i] = newnames[0]
                        newnames = newnames[1:]
        # report name changes
        for oldname,newname in zip(initial_fieldnames,fieldnames):
            if oldname != newname:
                self.AddWarning("Output name %s changed to %s (ambiguous or too long for shapefile format)"%(oldname,newname))
        return fieldnames

    @staticmethod
    def _name_stem(name):
        # has name already been fixed (ie does it end in underscore followed by a number?)
        if re.match(".*_[0-9]{1,}$",name):
            parts = name.split("_")
            stem = "_".join(parts[0:-1]) # everything before final underscore
        else:
            stem = name
        return stem

    @classmethod
    def _fix_name(cls,stem,count):
        decimal_places = len(str(count))
        format_string = "%%0%dd"%decimal_places
        suffices = [format_string%(x+1) for x in range(count)]
        stem = stem[0:(cls.max_field_length-1-decimal_places)]
        return [stem+"_"+x for x in suffices]

    def GetCreateCursor(self,outfilename,fieldnames,long_fieldnames,fieldtypes,geomtype):
        if geomtype=="NO_GEOM":
            return CSVCreateCursor(outfilename,long_fieldnames,self)
        else:
            return ShapefileCreateCursor(outfilename,fieldnames,long_fieldnames,fieldtypes,self,geomtype)

    def _create_shapefile(self,filename,fields,alii,fieldtypes,shapefileshapetype):
        fields = self._dbf_validate_fieldnames(fields)
        
        self.AddMessage("Adding fields:")
        format_string = "    %%-%ds - %%s"%self.max_field_length # aligns output nicely
        fieldfilename = "%s.names.csv"%filename
        fieldnames_out = open(fieldfilename,"w")
        for name,alias in zip(fields,alii):
            self.AddMessage(format_string%(name,alias))
            fieldnames_out.write("%s,%s\n"%(name,alias))
        fieldnames_out.close()
        self.AddMessage("Field names saved to %s"%fieldfilename)
        if self.projfile:
            shutil.copyfile(self.projfile,stripshpextensions(filename)+".prj")
        
        # create writer object and add requested fields
        writer = self.shapefile.Writer(shapefileshapetype)
        for n,t in zip(fields,fieldtypes):
            if t=="FLOAT":
                writer.field(n,"F",19,11) # add float field; 19 and 11 match what arc does for shape_length
            elif t=="INT":
                writer.field(n,"N")
            elif t=="STR":
                writer.field(n,"C")
            else:
                assert False

        return writer

class ShapefileCreateCursor(CreateCursor):

    def __init__(self,outfilename,fieldnames,alii,fieldtypes,env,geom_type):
        if len(fieldnames)==0:
            #add dummy data as pyshp can't cope with not having any :(
            fieldnames = ["dummy"]
            alii = ["dummy"]
            fieldtypes = ["INT"]
            self.dummydata=True
        else:
            self.dummydata=False
        self.outfilename = outfilename

        _str_to_shapefile_geom_type = { "POLYLINEZ": env.shapefile.POLYLINEZ,
                                        "POLYGON": env.shapefile.POLYGON,
                                        "MULTIPOLYLINEZ": env.shapefile.POLYLINEZ}
        shapefileshapetype = _str_to_shapefile_geom_type[geom_type]
        
        self.writer = env._create_shapefile(outfilename,fieldnames,alii,fieldtypes,shapefileshapetype)
        self.env = env
        
    def StartProgressor(self,numitems):
        self.numitems = numitems
        self.increment = get_sensible_increment(numitems)
        self.count = 0
        self.env.SetProgressor("step","Writing %s"%self.outfilename,numitems)
        
    def AddRowGeomItem(self,geomitem):
        if self.count%self.increment==0:
            self.env.SetProgressorPosition(self.count)
        self.count += 1
        
        geom = [x for x in geomitem.geom if x] # remove any blank parts
        if geom: # if it's not blank
            self.writer.line(geom)
            data = geomitem.data
            if self.dummydata:
                data = data + [0]
            assert data
            self.writer.record(*data)
            
    def Close(self):
        if (len(self.writer.shapes())):
            self.writer.save(self.outfilename)
            self.env.SetProgressorPosition(self.numitems)
        else:
            self.env.AddMessage("Shapefile %s not written - no shapes to write"%self.outfilename)
        del self.writer
        

class SdnaArcpyEnvironment(SdnaEnvironment):

    def get_out_base(self,outhandle):
        return outhandle
        
    def ensure_not_silly_path(self,gdb_path):
        if gdb_path and re.match(r".*\.gdb[^\\/].*",gdb_path.lower()):
            self.AddError("Odd path: "+gdb_path)
            self.AddError("This path contains '.gdb' followed by a character other than \\ or /")
            self.AddError("ArcGIS often crashes on such paths - rename your gdb or containing folders to fix it")
            sys.exit(1)
    
    def __init__(self,spatialreferencefcname):
        super(SdnaArcpyEnvironment, self).__init__()
        import arcpy
        self.arcpy = arcpy
        self.ensure_not_silly_path(spatialreferencefcname)
        self.spatialreferencefcname = spatialreferencefcname
        
    def ListFields(self,fcname):
        self.ensure_not_silly_path(fcname)
        return [x.name for x in self.arcpy.ListFields(fcname)]

    arc_type_to_type = { "SmallInteger":int, "Integer":int, "Float":float, "Single":float, "Double":float, "String":str, "Date":None, "OID":int, "Geometry":None, "Blob":None}

    def ListFieldTypes(self,fcname):
        self.ensure_not_silly_path(fcname)
        return [self.arc_type_to_type[x.type] for x in self.arcpy.ListFields(fcname)]

    def GetCount(self,fcname):
        self.ensure_not_silly_path(fcname)
        return int(self.arcpy.GetCount_management(fcname).getOutput(0))

    def ReadFeatures(self,fcname,fields):
        self.AddMessage("Reading features from %s"%fcname)
        self.ensure_not_silly_path(fcname)

        desc = self.arcpy.Describe(fcname)
        if not desc.hasOID:
            self.arcpy.AddError('Feature class has no object ID field')
            sys.exit(1)

        in_arc_idfield = desc.OIDfieldname
        shapefieldname = desc.ShapeFieldName
        has_z = desc.hasZ

        count = self.GetCount(fcname)
        self.SetProgressor("step","Reading features from %s"%fcname,count)
        increment = count/100 if count>100 else count
        rownum = 0
        with self.arcpy.da.SearchCursor(fcname,["OID@","SHAPE@"]+fields) as rows:
            for row in rows:
                if rownum%increment==0:
                    self.SetProgressorPosition(rownum)
                rownum += 1
                
                shape = row[1]
                arcid = row[0]
                fielddata=row[2:]

                pointlist = []
                if(shape.partCount != 1):
                    self.arcpy.AddError("Error: polyline %d has multiple parts"%arcid)
                    self.arcpy.AddError("Please run ArcToolbox -> Data Management")
                    self.arcpy.AddError("     -> Features -> Multipart to Singlepart")
                    self.arcpy.AddError(" to fix the input feature class before running sDNA")
                    sys.exit(1)

                for point in shape.getPart(0):
                    if has_z:
                        p = (point.X,point.Y,point.Z)
                    else:
                        p = (point.X,point.Y)
                    pointlist.append(p)

                yield arcid,pointlist,fielddata
        self.SetProgressorPosition(rownum)
        
    def GetCreateCursor(self,outfcname,fieldnames,long_fieldnames,fieldtypes,geomtype):
        if geomtype=="NO_GEOM":
            return CSVCreateCursor(outfcname,long_fieldnames,self)
        else:
            self.ensure_not_silly_path(outfcname)
            return ArcpyCreateCursor(outfcname,fieldnames,long_fieldnames,fieldtypes,self,geomtype)

class ArcpyCreateCursor(CreateCursor):

    def __init__(self,outfcname,fieldnames,fieldalii,fieldtypes,env,geomtype):
        self.outfcname = outfcname
        self.fieldnames = fieldnames
        
        self.env = env
        self.dontwrite = set()
        
        string_to_arc_geom_type_and_has_z = {"POLYGON":("POLYGON",False),"MULTIPOLYLINEZ":("POLYLINE",True),"POLYLINEZ":("POLYLINE",True)}

        gdb = os.path.dirname(outfcname)
        fcname = os.path.basename(outfcname)
        self.fullpath = gdb+u'\\'+fcname
        if self.env.arcpy.Exists(self.fullpath):
            self.env.AddError('Feature class %s already exists'%fcname)
            sys.exit(1)
        self.env.AddMessage("Creating feature class %s in %s"%(fcname,gdb))
        arc_geom_type,has_z = string_to_arc_geom_type_and_has_z[geomtype]
        self.has_z = has_z
        has_z_string = "ENABLED" if has_z else "DISABLED"
        self.env.AddMessage("Output z-values: %s"%has_z_string)
        self.env.arcpy.CreateFeatureclass_management(gdb,
                                   fcname,
                                   arc_geom_type,
                                   spatial_reference=env.spatialreferencefcname,
                                   has_z = has_z_string)

        python_to_arc_fieldtype = {"INT":"LONG","STR":"TEXT","FLOAT":"FLOAT",int:"LONG",str:"TEXT",float:"FLOAT"}
        
        existing_fieldnames = [f.name for f in self.env.arcpy.ListFields(outfcname)]
        
        self.fieldnames = [self.env.arcpy.ValidateFieldName(x,os.path.dirname(outfcname)) for x in self.fieldnames]
        for name,alias,datatype in zip(self.fieldnames,fieldalii,fieldtypes):
            if name not in existing_fieldnames:
                self.env.arcpy.AddField_management(outfcname,name,python_to_arc_fieldtype[datatype],field_alias=alias)
            else:
                # it must be a default, non writeable field in Arc
                self.dontwrite.add(name)

        self.ic = self.env.arcpy.InsertCursor(outfcname)
        self.line = self.env.arcpy.Array()
        self.pnt = self.env.arcpy.Point()
        self.count=0

    def StartProgressor(self,numitems):
        self.numitems = numitems
        self.increment = get_sensible_increment(numitems)
        self.env.SetProgressor("step","Writing %s"%self.outfcname,self.numitems)
        
    def AddRowGeomItem(self,geomitem):
        if self.count%self.increment==0:
            self.env.SetProgressorPosition(self.count)
        self.count += 1
        
        geom = [x for x in geomitem.geom if x] # remove empty parts
        if geom: # if non-empty
            lines = self.env.arcpy.Array()
            for part in geom:
                line = self.env.arcpy.Array()
                for point in part:
                    self.pnt.X, self.pnt.Y = point[0:2]
                    if len(point)==3 and self.has_z:
                        self.pnt.Z = point[2]
                    line.add(self.pnt)
                lines.add(line)

            # if it's a one-part multipart feature, convert to single part
            if len(geomitem.geom)==1:
                lines = lines[0]

            row = self.ic.newRow()
            row.shape = lines

            for name,value in zip(self.fieldnames,geomitem.data):
                if name not in self.dontwrite:
                    try:
                        row.setValue(name,value)
                    except:
                        row.setNull(name)
            
            self.ic.insertRow(row)
            del row
            
        

    def Close(self):
        del self.ic
        if self.count==0:
            self.env.arcpy.Delete_management(self.fullpath)
        if hasattr(self,"numitems"): #progressor was started
            self.env.SetProgressorPosition(self.numitems)

=======
# (c) Crispin Cooper on behalf of Cardiff University 2013-2015
# This file is released under MIT license

# This encapsulates the different environments in which the
# sDNA python scripts might run.
# At present there are two: 1. within ArcGIS; 2. directly on shapefiles

import abc,sys,os,re,shutil,csv
from os import path
from collections import defaultdict

def is_gdb(name):
    # anything that contains literal .gdb followed by / or \
    return bool(re.match(r".*\.gdb[/\\].*",name.lower()))

def stripshpextensions(name):
    ext = name[-4:].lower()
    if ext in [".shp",".shx",".dbf"]:
        return name[:-4]
    else:
        return name

def get_sensible_increment(numitems):
    inc = numitems/1000 if numitems>100 else numitems
    return inc if inc>0 else 1
        
class SdnaEnvironment(metaclass=abc.ABCMeta):
    def __init__(self):
        self.tty_unfinished_line = False
        self.progressor_output_100 = False
        self.is_interactive = sys.stdout.isatty()
        
    def SetProgressor(self,progressor_type,message,end):
        assert(progressor_type=="step" or progressor_type=="default")
        self.progressor_end = end
        self.AddMessage(message)
        if self.tty_unfinished_line:
            sys.stderr.write("\n")
            sys.stderr.flush()
        
    def SetProgressorPosition(self,position):
        if self.progressor_end==0:
            percent = 100
        else:
            percent = 100*float(position)/float(self.progressor_end)
        progressor_string = "%.1f%%"%percent
        if progressor_string != "100.0%":
            self.progressor_output_100 = False
            sys.stderr.write("\rProgress: %s"%progressor_string)
            self.tty_unfinished_line = True
        else:
            # only output "100%" once and follow it with a newline
            if not self.progressor_output_100:
                self.progressor_output_100 = True
                sys.stderr.write("\rProgress: 100%\n")
                self.tty_unfinished_line = False
        sys.stderr.flush()

    def ttyensurenewline(self):
        if self.tty_unfinished_line and self.is_interactive:
            sys.stderr.write("\n")
            self.tty_unfinished_line = False

    def AddWarning(self,warning):
        self.ttyensurenewline()
        sys.stdout.write(warning+"\n")

    def AddError(self,error):
        self.ttyensurenewline()
        sys.stdout.write(error+"\n")

    def AddMessage(self,message):
        self.ttyensurenewline()
        sys.stdout.write(message+"\n")

    @abc.abstractmethod
    def get_out_base(self,in_handle):
        return
    
    @abc.abstractmethod
    def ListFields(self,filename):
        return

    @abc.abstractmethod
    def ListFieldTypes(self,filename):
        return

    @abc.abstractmethod
    def GetCount(self,filename):
        return

    @abc.abstractmethod
    def ReadFeatures(self,filename,fields):
        return

    @abc.abstractmethod
    def GetCreateCursor(self,outfilename,fieldnames,long_fieldnames,fieldtypes,geomtype):
        return
        
    def ReadTableFile(self,source):
        class EnvTable(object):
            def __init__(self,name,zonefieldnames,env):
                if name.strip()=="":
                    env.AddError("Table column with no name")
                    sys.exit(1)
                self.name = name
                self.dims = len(zonefieldnames)
                self.zonefieldnames = zonefieldnames
                self.rowdata = []
                self.zones = []
                self.env = env
            def rows(self):
                for z,d in zip(self.zones,self.rowdata):
                    yield z,d
            def add(self,z,d,i):
                for zpart in z:
                    if zpart.strip()=="":
                        env.AddError("Zone with no name")
                self.zones += [z]
                try:
                    self.rowdata += [float(d)]
                except ValueError:
                    self.env.AddError('Bad data for variable "%s" row %d: "%s"'%(self.name,i,d)) 
                    sys.exit(1)
                
        def check_row_length(i,row,header):
            if len(row)<len(header):
                self.AddError("Row %d too short"%i)
                sys.exit(1)
            if len(row)>len(header):
                self.AddError("Row %d too long"%i)
                sys.exit(1)
                
        def csv_rstrip(line):
            while line[-1].strip()=="":
                line = line[:-1]
            return [x.strip() for x in line]
            
        class csv_rstrip_reader(object):
            def __init__(self,csvfile):
                self.reader = csv.reader(csvfile)
            def __next__(self):
                return csv_rstrip(next(self.reader))
            def __iter__(self):
                return self

        with open(source, 'rb') as csvfile: #https://stackoverflow.com/questions/40310042/python-read-csv-bom-embedded-into-the-first-key
            reader = csv_rstrip_reader(csvfile)
            metadata = next(reader)
            if metadata[0].lower()=="list": 
                if len(metadata)<2:
                    self.AddError("Table format error: number of zone fields not specified")
                    sys.exit(1)
                try:
                    dims = int(metadata[1])
                except ValueError:
                    self.AddError("Table format error: could not read number of zone fields")
                    sys.exit(1)
                if dims not in [1,2]:
                    self.AddError("Table format error: invalid number of zone fields (only 1 or 2 allowed)")
                    sys.exit(1)
                header = next(reader)
                if len(header)<dims+1:
                    self.AddError("Not enough columns")
                    sys.exit(1)
                tables = [EnvTable(name,header[0:dims],self) for name in header[dims:]]
                for i,row in enumerate(reader):
                    check_row_length(i,row,header)
                    zone = row[0:dims]
                    for t,d in zip(tables,row[dims:]):
                        t.add(zone,d,i)
                return tables
                        
            elif metadata[0].lower()=="matrix":
                if len(metadata)!=3:
                    self.AddError("Table format error: matrix metadata wrong length")
                    sys.exit(1)
                header = next(reader)
                table = EnvTable(header[0].lower(),metadata[1:3],self)
                to_zones = header[1:]
                for i,row in enumerate(reader):
                    check_row_length(i,row,header)
                    from_zone = row[0]
                    for to_zone,d in zip(to_zones,row[1:]):
                        table.add((from_zone,to_zone),d,i)
                return [table]
            
            else:
                self.AddError("Unknown table format: %s"%metadata[0])
                sys.exit(1)
            
                
class CreateCursor(metaclass=abc.ABCMeta):
    @abc.abstractmethod
    def AddRowGeomItem(self,geometryitem):
        return

    @abc.abstractmethod
    def StartProgressor(self,numitems):
        return
        
    @abc.abstractmethod
    def Close(self):
        return

class CSVCreateCursor(CreateCursor):
    def __init__(self,outfilename,long_fieldnames,env):
        self.env = env
        
        if outfilename.lower()[-4:]!=".csv":
            outfilename += ".csv"
            
        try:
            self.outfile = open(outfilename,"wb")
        except:
            env.AddError("Error opening output file "+outfilename)
            sys.exit(1)
            
        self.writer = csv.writer(self.outfile)
        self.writer.writerow(long_fieldnames)
    
    def Close(self):
        if hasattr(self,"outfile"):
            self.outfile.close()
        
    def AddRowGeomItem(self,geometryitem):
        def myformat(x):
            if type(x)==float:
                return ("%.6f"%x).rstrip("0")
            else:
                return x
                
        self.writer.writerow(list(map(myformat,geometryitem.data)))
        
    def StartProgressor(self,numitems):
        # no progressor for now as csv files are expected to be short
        return

class SdnaShapefileEnvironment(SdnaEnvironment):
    max_field_length = 10

    def get_out_base(self,outfilename):
        bits = outfilename.split(".")
        if bits[-1].lower()=="shp":
            del bits[-1]
        return ".".join(bits)

    def __init__(self,spatial_reference_source=""):
        super(SdnaShapefileEnvironment, self).__init__()
        import shapefile
        self.shapefile = shapefile
        projfile = stripshpextensions(spatial_reference_source)+".prj"
        if os.path.exists(projfile):
            self.projfile=projfile
        else:
            self.projfile=None
    
    def _sf_getfields(self,filename):
        try:
            sf = self.shapefile.Reader(filename)
        except:
            self.AddError("Can't open shapefile "+filename)
            sys.exit(1)
            
        fields = sf.fields[1:] # ignore deletion flag
        fieldnames = [x[0] for x in fields]
        fieldtypes = [SdnaShapefileEnvironment._sf_field_to_type(x[1]) for x in fields]
        return fieldnames,fieldtypes

    def ListFields(self,filename):
        return self._sf_getfields(filename)[0]

    def ListFieldTypes(self,filename):
        return self._sf_getfields(filename)[1]

    def GetCount(self,filename):
        sf = self.shapefile.Reader(filename)
        return sf.numRecords

    @staticmethod
    def _sf_field_to_type(letter):
        if letter=="F" or letter=="O":
            return float
        if letter=="L":
            return bool
        if letter=="N" or letter=="I" or letter=="+":
            return int
        if letter=="C" or letter=="D" or letter=="M":
            return str
        return str # default to string

    def ReadFeatures(self,filename,fields):
        self.AddMessage("Reading features from %s"%filename)
        sf = self.shapefile.Reader(filename)
        self.SetProgressor("step","Reading features from %s"%filename,sf.numRecords)
        increment = sf.numRecords/100 if sf.numRecords>100 else sf.numRecords
        data = sf.shapeRecords()
        fieldnames,fieldtypes = self._sf_getfields(filename)
        wanted_field_indices = [fieldnames.index(f) for f in fields]
        wanted_field_types = [fieldtypes[i] for i in wanted_field_indices]
        fid = 0
        for record in data:
            if fid%increment == 0:
                self.SetProgressorPosition(fid)
            fielddata=[]
            for i,typ in zip(wanted_field_indices,wanted_field_types):
                value = record.record[i]
                try:
                    converted_value = typ(value)
                except ValueError:
                    if re.match("^\\x00*$", value):
                        #bit weird but interpret uninitialized fields as default
                        #http://www.dbase.com/Knowledgebase/INT/db7_file_fmt.htm
                        converted_value = typ()
                    else:
                        name = fieldnames[i]
                        msg = "Failed to convert %s (in field %s line %s) to %s"%tuple(map(str,[value,name,fid,typ]))
                        self.AddError(msg)
                        sys.exit(1)
                fielddata += [converted_value]
            if list(record.shape.parts) != [0]:
                self.AddError("Multipart features are not supported in sDNA: convert your input data to single part features then rerun")
                sys.exit(1)
            points = record.shape.points
            if hasattr(record.shape,"z"):
                points = [(x,y,z) for (x,y),z in zip(points,record.shape.z)]
            yield fid,points,fielddata
            fid += 1
        self.SetProgressorPosition(fid)

    @classmethod
    def _spot_conflicting_name_stems(cls,fieldnames):
        name_count = defaultdict(int)
        for f in fieldnames:
            name_count[f] += 1
        matching_stems = [cls._name_stem(name) for name,count in list(name_count.items()) if count>1]
        # of course these may themselves contain duplicates - remove them
        matching_stems = list(dict([(x,None) for x in matching_stems]).keys())
        return matching_stems

    def _dbf_validate_fieldnames(self,fieldnames):
        # truncate, then check for conflicts
        initial_fieldnames = fieldnames
        fieldnames = [x[0:self.max_field_length] for x in fieldnames]
        while True:
            conflicts = self._spot_conflicting_name_stems(fieldnames)
            if not conflicts:
                break
            # replace conflicting names
            #conflicts are 1. any matching names, 2. any names whose stem matches the stem of other conflicts
            for stemname in conflicts:
                must_replace = [self._name_stem(x)==stemname for x in fieldnames]
                count = len([x for x in must_replace if x])
                newnames = self._fix_name(stemname,count)
                for i,(name,must_replace) in enumerate(zip(fieldnames,must_replace)):
                    if must_replace:
                        fieldnames[i] = newnames[0]
                        newnames = newnames[1:]
        # report name changes
        for oldname,newname in zip(initial_fieldnames,fieldnames):
            if oldname != newname:
                self.AddWarning("Output name %s changed to %s (ambiguous or too long for shapefile format)"%(oldname,newname))
        return fieldnames

    @staticmethod
    def _name_stem(name):
        # has name already been fixed (ie does it end in underscore followed by a number?)
        if re.match(".*_[0-9]{1,}$",name):
            parts = name.split("_")
            stem = "_".join(parts[0:-1]) # everything before final underscore
        else:
            stem = name
        return stem

    @classmethod
    def _fix_name(cls,stem,count):
        decimal_places = len(str(count))
        format_string = "%%0%dd"%decimal_places
        suffices = [format_string%(x+1) for x in range(count)]
        stem = stem[0:(cls.max_field_length-1-decimal_places)]
        return [stem+"_"+x for x in suffices]

    def GetCreateCursor(self,outfilename,fieldnames,long_fieldnames,fieldtypes,geomtype):
        if geomtype=="NO_GEOM":
            return CSVCreateCursor(outfilename,long_fieldnames,self)
        else:
            return ShapefileCreateCursor(outfilename,fieldnames,long_fieldnames,fieldtypes,self,geomtype)

    def _create_shapefile(self,filename,fields,alii,fieldtypes,shapefileshapetype):
        fields = self._dbf_validate_fieldnames(fields)
        
        fields = [str(f,"utf-8") for f in fields]
        alii = [str(f,"utf-8") for f in alii]
        
        self.AddMessage("Adding fields:")
        format_string = "    %%-%ds - %%s"%self.max_field_length # aligns output nicely
        fieldfilename = "%s.names.csv"%filename
        fieldnames_out = open(fieldfilename,"w")
        for name,alias in zip(fields,alii):
            self.AddMessage(format_string%(name,alias))
            fieldnames_out.write("%s,%s\n"%(name,alias))
        fieldnames_out.close()
        self.AddMessage("Field names saved to %s"%fieldfilename)
        if self.projfile:
            shutil.copyfile(self.projfile,stripshpextensions(filename)+".prj")
        
        # create writer object and add requested fields
        writer = self.shapefile.Writer(filename,shapeType=shapefileshapetype)
        for n,t in zip(fields,fieldtypes):
            if t==b"FLOAT":
                writer.field(n,"F",19,11) # add float field; 19 and 11 match what arc does for shape_length
            elif t==b"INT":
                writer.field(n,"N")
            elif t==b"STR":
                writer.field(n,"C")
            else:
                assert False

        return writer

class ShapefileCreateCursor(CreateCursor):

    def __init__(self,outfilename,fieldnames,alii,fieldtypes,env,geom_type):
        if len(fieldnames)==0:
            #add dummy data as pyshp can't cope with not having any :(
            fieldnames = ["dummy"]
            alii = ["dummy"]
            fieldtypes = ["INT"]
            self.dummydata=True
        else:
            self.dummydata=False
        self.outfilename = outfilename

        _str_to_shapefile_geom_type = { b"POLYLINEZ": env.shapefile.POLYLINEZ,
                                        b"POLYGON": env.shapefile.POLYGON,
                                        b"MULTIPOLYLINEZ": env.shapefile.POLYLINEZ}
        shapefileshapetype = _str_to_shapefile_geom_type[geom_type]
        
        self.writer = env._create_shapefile(outfilename,fieldnames,alii,fieldtypes,shapefileshapetype)
        
        _str_to_geom_write_method = { b"POLYLINEZ": self.writer.linez,
                                        b"POLYGON": self.writer.poly,
                                        b"MULTIPOLYLINEZ": self.writer.linez}
        
        self.geom_write_method = _str_to_geom_write_method[geom_type]
        self.env = env
        
    def StartProgressor(self,numitems):
        self.numitems = numitems
        self.increment = get_sensible_increment(numitems)
        self.count = 0
        self.env.SetProgressor("step","Writing %s"%self.outfilename,numitems)
        
    def AddRowGeomItem(self,geomitem):
        if self.count%self.increment==0:
            self.env.SetProgressorPosition(self.count)
        self.count += 1
        
        geom = [x for x in geomitem.geom if x] # remove any blank parts
        
        if geom: # if it's not blank
            self.geom_write_method(geom)
            data = geomitem.data
            if self.dummydata:
                data = data + [0]
            assert data
            self.writer.record(*data)
            
    def Close(self):
        self.env.SetProgressorPosition(self.numitems)
        del self.writer

class SdnaArcpyEnvironment(SdnaEnvironment):

    def get_out_base(self,outhandle):
        return outhandle
        
    def ensure_not_silly_path(self,gdb_path):
        if gdb_path and re.match(r".*\.gdb[^\\/].*",gdb_path.lower()):
            self.AddError("Odd path: "+gdb_path)
            self.AddError("This path contains '.gdb' followed by a character other than \\ or /")
            self.AddError("ArcGIS often crashes on such paths - rename your gdb or containing folders to fix it")
            sys.exit(1)
    
    def __init__(self,spatialreferencefcname):
        super(SdnaArcpyEnvironment, self).__init__()
        import arcpy
        self.arcpy = arcpy
        self.ensure_not_silly_path(spatialreferencefcname)
        self.spatialreferencefcname = spatialreferencefcname
        
    def ListFields(self,fcname):
        self.ensure_not_silly_path(fcname)
        return [x.name for x in self.arcpy.ListFields(fcname)]

    arc_type_to_type = { "SmallInteger":int, "Integer":int, "Float":float, "Single":float, "Double":float, "String":str, "Date":None, "OID":int, "Geometry":None, "Blob":None}

    def ListFieldTypes(self,fcname):
        self.ensure_not_silly_path(fcname)
        return [self.arc_type_to_type[x.type] for x in self.arcpy.ListFields(fcname)]

    def GetCount(self,fcname):
        self.ensure_not_silly_path(fcname)
        return int(self.arcpy.GetCount_management(fcname).getOutput(0))

    def ReadFeatures(self,fcname,fields):
        self.AddMessage("Reading features from %s"%fcname)
        self.ensure_not_silly_path(fcname)

        desc = self.arcpy.Describe(fcname)
        if not desc.hasOID:
            self.arcpy.AddError('Feature class has no object ID field')
            sys.exit(1)

        in_arc_idfield = desc.OIDfieldname
        shapefieldname = desc.ShapeFieldName
        has_z = desc.hasZ

        count = self.GetCount(fcname)
        self.SetProgressor("step","Reading features from %s"%fcname,count)
        increment = count/100 if count>100 else count
        rownum = 0
        with self.arcpy.da.SearchCursor(fcname,["OID@","SHAPE@"]+fields) as rows:
            for row in rows:
                if rownum%increment==0:
                    self.SetProgressorPosition(rownum)
                rownum += 1
                
                shape = row[1]
                arcid = row[0]
                fielddata=row[2:]

                pointlist = []
                if(shape.partCount != 1):
                    self.arcpy.AddError("Error: polyline %d has multiple parts"%arcid)
                    self.arcpy.AddError("Please run ArcToolbox -> Data Management")
                    self.arcpy.AddError("     -> Features -> Multipart to Singlepart")
                    self.arcpy.AddError(" to fix the input feature class before running sDNA")
                    sys.exit(1)

                for point in shape.getPart(0):
                    if has_z:
                        p = (point.X,point.Y,point.Z)
                    else:
                        p = (point.X,point.Y)
                    pointlist.append(p)

                yield arcid,pointlist,fielddata
        self.SetProgressorPosition(rownum)
        
    def GetCreateCursor(self,outfcname,fieldnames,long_fieldnames,fieldtypes,geomtype):
        if geomtype=="NO_GEOM":
            return CSVCreateCursor(outfcname,long_fieldnames,self)
        else:
            self.ensure_not_silly_path(outfcname)
            return ArcpyCreateCursor(outfcname,fieldnames,long_fieldnames,fieldtypes,self,geomtype)

class ArcpyCreateCursor(CreateCursor):

    def __init__(self,outfcname,fieldnames,fieldalii,fieldtypes,env,geomtype):
        self.outfcname = outfcname
        self.fieldnames = fieldnames
        
        self.env = env
        self.dontwrite = set()
        
        string_to_arc_geom_type_and_has_z = {"POLYGON":("POLYGON",False),"MULTIPOLYLINEZ":("POLYLINE",True),"POLYLINEZ":("POLYLINE",True)}

        gdb = os.path.dirname(outfcname)
        fcname = os.path.basename(outfcname)
        self.fullpath = gdb+'\\'+fcname
        if self.env.arcpy.Exists(self.fullpath):
            self.env.AddError('Feature class %s already exists'%fcname)
            sys.exit(1)
        self.env.AddMessage("Creating feature class %s in %s"%(fcname,gdb))
        arc_geom_type,has_z = string_to_arc_geom_type_and_has_z[geomtype]
        self.has_z = has_z
        has_z_string = "ENABLED" if has_z else "DISABLED"
        self.env.AddMessage("Output z-values: %s"%has_z_string)
        self.env.arcpy.CreateFeatureclass_management(gdb,
                                   fcname,
                                   arc_geom_type,
                                   spatial_reference=env.spatialreferencefcname,
                                   has_z = has_z_string)

        python_to_arc_fieldtype = {"INT":"LONG","STR":"TEXT","FLOAT":"FLOAT",int:"LONG",str:"TEXT",float:"FLOAT"}
        
        existing_fieldnames = [f.name for f in self.env.arcpy.ListFields(outfcname)]
        
        self.fieldnames = [self.env.arcpy.ValidateFieldName(x,os.path.dirname(outfcname)) for x in self.fieldnames]
        for name,alias,datatype in zip(self.fieldnames,fieldalii,fieldtypes):
            if name not in existing_fieldnames:
                self.env.arcpy.AddField_management(outfcname,name,python_to_arc_fieldtype[datatype],field_alias=alias)
            else:
                # it must be a default, non writeable field in Arc
                self.dontwrite.add(name)

        self.ic = self.env.arcpy.InsertCursor(outfcname)
        self.line = self.env.arcpy.Array()
        self.pnt = self.env.arcpy.Point()
        self.count=0

    def StartProgressor(self,numitems):
        self.numitems = numitems
        self.increment = get_sensible_increment(numitems)
        self.env.SetProgressor("step","Writing %s"%self.outfcname,self.numitems)
        
    def AddRowGeomItem(self,geomitem):
        if self.count%self.increment==0:
            self.env.SetProgressorPosition(self.count)
        self.count += 1
        
        geom = [x for x in geomitem.geom if x] # remove empty parts
        if geom: # if non-empty
            lines = self.env.arcpy.Array()
            for part in geom:
                line = self.env.arcpy.Array()
                for point in part:
                    self.pnt.X, self.pnt.Y = point[0:2]
                    if len(point)==3 and self.has_z:
                        self.pnt.Z = point[2]
                    line.add(self.pnt)
                lines.add(line)

            # if it's a one-part multipart feature, convert to single part
            if len(geomitem.geom)==1:
                lines = lines[0]

            row = self.ic.newRow()
            row.shape = lines

            for name,value in zip(self.fieldnames,geomitem.data):
                if name not in self.dontwrite:
                    try:
                        row.setValue(name,value)
                    except:
                        row.setNull(name)
            
            self.ic.insertRow(row)
            del row
            
        

    def Close(self):
        del self.ic
        if self.count==0:
            self.env.arcpy.Delete_management(self.fullpath)
        if hasattr(self,"numitems"): #progressor was started
            self.env.SetProgressorPosition(self.numitems)

>>>>>>> 52f74569... Convert arcscripts folder to Python 3.
