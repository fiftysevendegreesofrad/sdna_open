# (c) Crispin Cooper on behalf of Cardiff University 2015
# This file is released under MIT license

import _parentdir
import sdna_environment
from sdnaregutilities import *

import optparse,re
from optparse import OptionParser

example='python -u sdnalearn.py --calibfile cfile --target var0 --vars var1,var2 --varregex "var.*"\n'
desc = 'Examples:\n'\
       + example\
       + "GDB usage requires ArcGIS to be installed, Shapefile usage doesn't."

op = OptionParser("usage: %prog [options] [config_string]",epilog= desc)

op.add_option("--calibfile",dest="calibfile",
              help="Calibration file",metavar="FILE")
op.add_option("--target",dest="target",help="Target variable",metavar="VAR")
op.add_option("--vars",dest="vars",help="varibles to test",metavar="COMMA_SEPARATED_LIST")
op.add_option("--varregex",dest="varregex",help="Variable regex",metavar="REGEX")
op.add_option("--boxcoxtarget",dest="boxcoxtarget",help="Box-Cox transform the target variable",action="store_true",default=False)
#op.add_option("--btregex",dest="btregex",help="Regex specifying which predictor variables to transform using Box-Tidwell",metavar="REGEX",default="")
op.add_option("--bcregex",dest="bcregex",help="Regex specifying which predictor variables to transform using Box-Cox",metavar="REGEX",default="")
op.add_option("--mode",dest="mode",help="Mode",metavar=MODES,default=SINGLE_BEST)
op.add_option("--weightlambda",dest="weightlambda",help="Lambda for weighting",metavar="L",default=1,type="float")
op.add_option("--nfolds",dest="nfolds",help="Number of folds for cross-validation",metavar="N",default=7,type="int")
op.add_option("--reps",dest="reps",help="Number of times to repeat N-fold cross-validation",metavar="REPS",default=50,type="int")
op.add_option("--gehtime",dest="gehtime",help="Time interval in hours for GEH",metavar="HOURS",default=1,type="float")
op.add_option("--output",dest="output",help="Output table",metavar="FILE")
op.add_option("--intercept",dest="intercept",help="Use intercept (multivariate models only)",action="store_true",default=False)
op.add_option("--resids",dest="resids",help="Output path for residuals",metavar="PATH",default="")
op.add_option("--reglambdaminmax",dest="reglambda",help="Regularization lambda min,max",metavar="MIN,MAX",default="")

# or use input and output maps
op.add_option("--im",dest="im",help=optparse.SUPPRESS_HELP)
op.add_option("--om",dest="om",help=optparse.SUPPRESS_HELP)
              
(options,args) = op.parse_args()

# disable box tidwell
options.btregex = ""

def to_map(s):
    m = {}
    for pair in s.split(";"):
        try:
            key,val = pair.split("=")
        except ValueError:
            op.error("Bad map specifier: "+s)
        m[key.strip()]=val.strip()
    return m

if options.im:
    assert (options.om)
    inputs = to_map(options.im)
    options.calibfile = inputs["calibfile"]
    outputs = to_map(options.om)
    options.output = outputs["modelout"]
    options.resids = outputs["resids"]

# check required options are present

if len(args)>0:
    op.error("Trailing arguments on command line")

options.mode = options.mode.lower()
if options.mode not in MODES.split("|"):
    op.error("Unrecognized mode")
    
for thing,name in [(options.calibfile,"calibration file"),(options.target,"target variable")]:
    if thing==None:
        op.error(name+" not specified")
        
if options.vars==None and options.varregex==None:
    op.error("Must specify one of --vars or --varregex")
        
is_gdb = [sdna_environment.is_gdb(x) for x in [options.calibfile,options.resids]]
tolerance_source = options.calibfile
    
if any(is_gdb):
    env = sdna_environment.SdnaArcpyEnvironment(tolerance_source)
else:
    env = sdna_environment.SdnaShapefileEnvironment(tolerance_source)

# determine fieldnames and check present
    
fieldnames = env.ListFields(options.calibfile)
fieldtypes = env.ListFieldTypes(options.calibfile)
numeric_fields = [fn for fn,ft in zip(fieldnames,fieldtypes) if ft in [int,float]]

listed_fields = re.split(",|;",options.vars) if options.vars else []
for wf in listed_fields+[options.target]:
    if wf not in fieldnames:
        op.error(wf+" is not a field in "+options.calibfile)
    if wf not in numeric_fields:
        op.error(wf+" is not a numeric field in "+options.calibfile)
        
try:
    regex = re.compile(options.varregex) if options.varregex else None
except Exception as e:
    op.error("Error in variable regex: "+e.message)

try:
    btregex = re.compile(options.btregex) if options.btregex else None
except Exception as e:
    op.error("Error in Box Tidwell regex: "+e.message)

try:
    bcregex = re.compile(options.bcregex) if options.bcregex else None
except Exception as e:
    op.error("Error in Box Cox regex: "+e.message)
    
regex_fields = [fn for fn in numeric_fields if regex.match(fn) and fn not in listed_fields] if regex else []

env.AddMessage("Requested fields: "+",".join(listed_fields))
env.AddMessage("Fields matching regex: "+",".join(regex_fields))

wanted_fields = [fn for fn in listed_fields+regex_fields if fn!=options.target]

# read data

data = []
targetdata = []
ids = []
shapes = []
for id,shape,dataline in env.ReadFeatures(options.calibfile,wanted_fields+[options.target]):
    data += [dataline[:-1]]
    targetdata += [dataline[-1]]
    ids += [id]
    shapes += [shape]

# check suitable for modelling

if numpy.std(targetdata)==0:
    op.error("Target variable had no variance")

if len(wanted_fields)==0:
    op.error("No predictor variables given!")
    
usable_data = {}
variable_order = []
for i,name in enumerate(wanted_fields):
    values = [line[i] for line in data]
    if numpy.std(values)==0:
        env.AddWarning('Discarding field with no variation: '+name+' is all '+str(values[0]))
    else:
        usable_data[name] = values
        variable_order += [name]
        
if len(usable_data)==0:
    op.error("Discarded all predictor variables due to no variation!")

if options.reglambda=="":
    reglambda = None
else:
    reglambda = list(map(float,options.reglambda.split(",")))
    assert len(reglambda)==2
    
# create model

model,preds,gehs = SdnaRegModel.fromData(targetdata,
                                         usable_data,
                                         variable_order,
                                         options.target,
                                         options.mode,
                                         options.nfolds,
                                         options.reps,
                                         options.boxcoxtarget,
                                         btregex,
                                         bcregex,
                                         env,
                                         options.weightlambda,
                                         options.gehtime,
                                         options.intercept,
                                         reglambda)

env.AddMessage("\n"+model.__repr__()+"\n")

if options.output:
    env.AddMessage("Saving model to "+options.output)
    model.save(options.output)
    
if options.resids:
    env.AddMessage("Saving residuals to "+options.resids)

    resid_fields = [options.target,"prediction","geh"]

    cc = env.GetCreateCursor(options.resids,resid_fields,resid_fields,["FLOAT"]*len(resid_fields),"POLYLINEZ")
    cc.StartProgressor(len(ids))
    
    class Item():
        pass
    for id,shape,target,pred,geh in zip(ids,shapes,targetdata,preds,gehs):
        if target<pred:
            geh = -geh
            
        item = Item()
        item.id = id
        item.geom = [shape]
        item.data = [target,pred,geh]
        item.textdata = []
        cc.AddRowGeomItem(item)
        
    cc.Close()