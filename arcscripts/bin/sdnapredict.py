# (c) Crispin Cooper on behalf of Cardiff University 2015
# This file is released under MIT license

import _parentdir
import sdna_environment
from sdnaregutilities import *

import optparse
from optparse import OptionParser

example='python -u sdnapredict.py --input infile --output outfile --modelfile model.csv --predvarname prediction\n'
desc = 'Examples:\n'\
       + example\
       + "GDB usage requires ArcGIS to be installed, Shapefile usage doesn't."

op = OptionParser("usage: %prog [options] [config_string]",epilog= desc)

op.add_option("--modelfile",dest="modelfile",
              help="Lerning model file",metavar="FILE")
op.add_option("--input",dest="input",
              help="Input data for prediction",metavar="FILE")
op.add_option("--predvarname",dest="target",help="Prediction variable name",metavar="VAR")
op.add_option("--output",dest="output",help="Output file",metavar="FILE")
              
# or use input and output maps
op.add_option("--im",dest="im",help=optparse.SUPPRESS_HELP)
op.add_option("--om",dest="om",help=optparse.SUPPRESS_HELP)
              
(options,args) = op.parse_args()

if options.im:
    assert (options.om)
    inputs = options.im.split(";")
    assert len(inputs)==1
    k,v = inputs[0].split("=")
    options.input = v
    outputs = options.om.split(";")
    assert len(outputs)==1
    k,v = outputs[0].split("=")
    options.output = v

# check required options are present

if len(args)>0:
    op.error("Trailing arguments on command line")

for thing,name in [(options.modelfile,"model file"),
                    (options.target,"prediction variable"),
                    (options.input,"prediction input"),
                    (options.output,"output")]:
    if thing==None:
        op.error(name+" not specified")
        
is_gdb = [sdna_environment.is_gdb(x) for x in [options.input,options.output]]
tolerance_source = options.input
    
if any(is_gdb):
    env = sdna_environment.SdnaArcpyEnvironment(tolerance_source)
else:
    env = sdna_environment.SdnaShapefileEnvironment(tolerance_source)

# load model file

model = SdnaRegModel.fromFile(options.modelfile)
wanted_fields = model.getVarNames() 
    
# determine fieldnames and check present
    
fieldnames = env.ListFields(options.input)
fieldtypes = env.ListFieldTypes(options.input)
numeric_fields = [fn for fn,ft in zip(fieldnames,fieldtypes) if ft in [int,float]]

for wf in wanted_fields:
    if wf not in fieldnames:
        op.error(wf+" is not a field in "+options.input)
    if wf not in numeric_fields:
        op.error(wf+" is not a numeric field in "+options.input)
        
# read data

data = []
ids=[]
shapes=[]
        
for id,shape,dataline in env.ReadFeatures(options.input,wanted_fields):
    data += [dataline]
    ids += [id]
    shapes += [shape]

data = numpy.array(data) # indexed by row then var

# predict

predictions = model.predict(data)
data = numpy.concatenate((data.T,[predictions])).T
wanted_fields += [options.target]

# output

cc = env.GetCreateCursor(options.output,wanted_fields,wanted_fields,["FLOAT"]*len(wanted_fields),"POLYLINEZ")
cc.StartProgressor(len(ids))
class Item():
    pass
for id,shape,dataline in zip(ids,shapes,data):
    item = Item()
    item.id = id
    item.geom = [shape]
    item.data = list(dataline)
    item.textdata = []
    cc.AddRowGeomItem(item)
cc.Close()    