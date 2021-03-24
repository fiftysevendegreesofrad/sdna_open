'''This file defines user interfaces to sDNA tools and how to convert inputs to config'''

##This file (and this file only) is released under the MIT license
##
##The MIT License (MIT)
##
##Copyright (c) 2015 Cardiff University
##
##Permission is hereby granted, free of charge, to any person obtaining a copy
##of this software and associated documentation files (the "Software"), to deal
##in the Software without restriction, including without limitation the rights
##to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
##copies of the Software, and to permit persons to whom the Software is
##furnished to do so, subject to the following conditions:
##
##The above copyright notice and this permission notice shall be included in
##all copies or substantial portions of the Software.
##
##THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
##IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
##FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
##AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
##LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
##OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
##THE SOFTWARE.

def metric_dropdown(name,label,include_match_analytical=False):
    optlist = ["EUCLIDEAN","ANGULAR","CUSTOM","CYCLE","CYCLE_ROUNDTRIP","EUCLIDEAN_ANGULAR"]
    return (name,label,"Text",optlist,optlist[0],True)

# when this changes, add it to geodesics etc
def weighting_options():
    return [("weighting","Weighting","Text",["Link","Length","Polyline"],"Link",True),
            ("origweight","Origin weight","Field",("Numeric","input"),"",False),
            ("destweight","Destination weight","Field",("Numeric","input"),"",False)]
            
def weighting_config(args):
    return "origweight=%(origweight)s;destweight=%(destweight)s;weight_type=%(weighting)s"%args
    
def radius_options(include_banded = True,include_cont = True):
    retval = [("radii","Radii (in units of source data projection)","Text",None,"n",True)]
    if include_banded:
        retval += [("bandedradii","Banded radius","Bool",None,False,False)]
    if include_cont:
        retval += [("cont","Continuous Space","Bool",None,False,False)]
    return retval
            
def radius_config(args):
    retval = ";radii=%(radii)s;"%args
    if "bandedradii" in args and args["bandedradii"]:
        retval += "bandedradii;"
    if "cont" in args and args["cont"]:
        retval += "cont;"
    return retval
    
def quote(x):
    return '"'+x+'"'

class sDNAIntegral(object):
    alias = "Integral Analysis"
    desc = \
"""<p>sDNA Integral is the core analysis tool of sDNA.  It computes several flow, accessibility, severance and efficiency measures on networks.
<p>For full details, see the sDNA documentation.
"""
    category = "Analysis"
        
    def getInputSpec(self):
        return [("input","Input polyline features","FC","Polyline","",True),
                  ("output","Output features","OFC",None,"",True),
                  ("betweenness","Compute betweenness","Bool",None,True,False),
                  ("bidir","Betweenness is bidirectional","Bool",None,False,False),
                  ("junctions","Compute junction counts","Bool",None,False,False),
                  ("hull","Compute convex hull statistics","Bool",None,False,False),
                  ("start_gs","Start grade separation","Field",("Numeric","input"),"",False),
                  ("end_gs","End grade separation","Field",("Numeric","input"),"",False),
                  metric_dropdown("analmet","Routing and analysis metric")]\
                  +radius_options()\
                  +weighting_options()\
                  +[
                      ("zonefiles","Zone table input csv files","MultiInFile","csv","",False),
                      ("odfile","Origin Destination Matrix input file","InFile","csv","",False),
                      ("custommetric","Custom metric field","Field",("Numeric","input"),"",False),
                      ("disable","Disable lines (field name or expression)","Text",None,"",False),
                      ("oneway","One way restrictions","Field",("Numeric","input"),"",False),
                      ("intermediates","Intermediate link filter (field name or expression)","Text",None,"",False),
                      ("advanced","Advanced config","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnaintegral"
        syntax["inputs"] = {"net":args["input"]}
        syntax["outputs"] = {"net":args["output"]}
        syntax["config"] = "start_gs=%(start_gs)s;end_gs=%(end_gs)s;"\
                           "metric=%(analmet)s;"\
                           "custommetric=%(custommetric)s;disable=%(disable)s;intermediates=%(intermediates)s;oneway=%(oneway)s;"\
                           "%(advanced)s;"%args\
                           + weighting_config(args) + radius_config(args)
        for arg,conf,invert in [("betweenness","nobetweenness",True),("junctions","nojunctions",True),("hull","nohull",True),("bidir","bidir",False)]:
            boolval = args[arg]
            if invert:
                boolval = not boolval
            if boolval:
                syntax["config"]+=";%s"%conf
                
        syntax["inputs"]["tables"]=""
        if args["odfile"]!="":
            syntax["config"]+=";odmatrix"
            syntax["inputs"]["tables"]+=args["odfile"]
        if args["zonefiles"]!="":
            syntax["inputs"]["tables"]=",".join([syntax["inputs"]["tables"]]+args["zonefiles"].split(";"))
                
        syntax["config"] = quote(syntax["config"])
        return syntax   

class sDNAIntegralFromOD(object):
    alias = "Integral from OD Matrix (assignment model)"
    desc = \
"""<p>Runs Integral Analysis from a pre-specified Origin-Destination Matrix, allowing import from other models.
"""
    category = "Analysis"
        
    def getInputSpec(self):
        return [("input","Input polyline features","FC","Polyline","",True),
                  ("odfile","Origin Destination Matrix input file","InFile","csv","",True),
                  ("output","Output features","OFC",None,"",True),
                  ("bidir","Betweenness is bidirectional","Bool",None,False,False),
                  ("start_gs","Start grade separation","Field",("Numeric","input"),"",False),
                  ("end_gs","End grade separation","Field",("Numeric","input"),"",False),
                  metric_dropdown("analmet","Routing and analysis metric"),
                  ("custommetric","Custom metric field","Field",("Numeric","input"),"",False),
                  ("disable","Disable lines (field name or expression)","Text",None,"",False),
                  ("oneway","One way restrictions","Field",("Numeric","input"),"",False),
                  ("intermediates","Intermediate link filter (field name or expression)","Text",None,"",False),
                  ("zonedist","Zone weight distribution expression","Text",None,"euc",True),
                  ("advanced","Advanced config","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnaintegral"
        syntax["inputs"] = {"net":args["input"],"tables":args["odfile"]}
        syntax["outputs"] = {"net":args["output"]}
        syntax["config"] = "odmatrix;zonedist=%(zonedist)s;start_gs=%(start_gs)s;end_gs=%(end_gs)s;"\
                           "metric=%(analmet)s;nojunctions;nohull;radii=n;"\
                           "custommetric=%(custommetric)s;disable=%(disable)s;intermediates=%(intermediates)s;oneway=%(oneway)s;"\
                           "%(advanced)s;"%args
                           
        for arg,conf,invert in [("bidir","bidir",False)]:
            boolval = args[arg]
            if invert:
                boolval = not boolval
            if boolval:
                syntax["config"]+=";%s"%conf
        
        syntax["config"] = quote(syntax["config"])        
        return syntax

     
        
class sDNASkim(object):
    alias = "Skim Matrix"
    desc = \
"""<p>Captures mean distance between zones as a skim matrix for input into external modelling tools.
"""
    category = "Analysis"
        
    def getInputSpec(self):
        return [("input","Input polyline features","FC","Polyline","",True),
                ("output","Output Skim Matrix File","OutFile","csv","",True),
                  ("skimorigzone","Origin zone field","Field",("String","input"),"",True),
                  ("skimdestzone","Destination zone field","Field",("String","input"),"",True),
                  ("start_gs","Start grade separation","Field",("Numeric","input"),"",False),
                  ("end_gs","End grade separation","Field",("Numeric","input"),"",False),
                  metric_dropdown("analmet","Routing and analysis metric"),
                  ("custommetric","Custom metric field","Field",("Numeric","input"),"",False)]\
                  +weighting_options()\
                  +[("disable","Disable lines (field name or expression)","Text",None,"",False),
                  ("oneway","One way restrictions","Field",("Numeric","input"),"",False),
                  ("odfile","Origin Destination Matrix input file","InFile","csv","",False),
                  ("zonefiles","Zone table input csv files","MultiInFile","csv","",False),
                  ("advanced","Advanced config","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnaintegral"
        syntax["inputs"] = {"net":args["input"]}
        syntax["outputs"] = {"skim":args["output"]}
        syntax["config"] = "outputskim;skipzeroweightorigins;skimorigzone=%(skimorigzone)s;skimdestzone=%(skimdestzone)s;start_gs=%(start_gs)s;end_gs=%(end_gs)s;radii=n;nobetweenness;nojunctions;nohull;nonetdata;"\
                           "metric=%(analmet)s;"\
                           "custommetric=%(custommetric)s;disable=%(disable)s;oneway=%(oneway)s;"\
                           "%(advanced)s;"%args\
                           + weighting_config(args)
        
        syntax["inputs"]["tables"]=""
        if args["odfile"]!="":
            syntax["config"]+=";odmatrix"
            syntax["inputs"]["tables"]+=args["odfile"]
        if args["zonefiles"]!="":
            syntax["inputs"]["tables"]=",".join([syntax["inputs"]["tables"]]+args["zonefiles"].split(";"))
        
        syntax["config"] = quote(syntax["config"])        
        return syntax

        
class sDNAGeodesics(object):
    alias = "Geodesics"
    desc = "<p>Outputs the geodesics (shortest paths) used by sDNA Integral analysis."
    category = "Analysis geometry"
    
    def getInputSpec(self):
        return [("input","Input polyline features","FC","Polyline","",True),
                  ("output","Output geodesic polyline features","OFC",None,"",True),
                  ("start_gs","Start grade separation","Field",("Numeric","input"),"",False),
                  ("end_gs","End grade separation","Field",("Numeric","input"),"",False),
                  ("origins","Origin IDs (leave blank for all)","Text",None,"",False),
                  ("destinations","Destination IDs (leave blank for all)","Text",None,"",False),
                  metric_dropdown("analmet","Routing and analysis metric"),
                  ("custommetric","Custom metric field","Field",("Numeric","input"),"",False)]\
                  +weighting_options()+\
                  [("odfile","Origin Destination Matrix input file","InFile","csv","",False),
                  ("zonefiles","Zone table input csv files","MultiInFile","csv","",False)]\
                  +radius_options()+\
                  [("disable","Disable lines (field name or expression)","Text",None,"",False),
                  ("oneway","One way restrictions","Field",("Numeric","input"),"",False),
                  ("intermediates","Intermediate link filter (field name or expression)","Text",None,"",False),
                  ("advanced","Advanced config","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnaintegral"
        syntax["inputs"] = {"net":args["input"]}
        syntax["outputs"] = {"geodesics":args["output"]}
        syntax["config"] = "start_gs=%(start_gs)s;end_gs=%(end_gs)s;"\
                           "metric=%(analmet)s;"\
                           "custommetric=%(custommetric)s;"\
                           "nonetdata;outputgeodesics;"\
                           "origins=%(origins)s;destinations=%(destinations)s;disable=%(disable)s;oneway=%(oneway)s;intermediates=%(intermediates)s;"\
                           "%(advanced)s;"%args\
                           +weighting_config(args) + radius_config(args)
        
        syntax["inputs"]["tables"]=""
        if args["odfile"]!="":
            syntax["config"]+=";odmatrix"
            syntax["inputs"]["tables"]+=args["odfile"]
        if args["zonefiles"]!="":
            syntax["inputs"]["tables"]=",".join([syntax["inputs"]["tables"]]+args["zonefiles"].split(";"))
        
        syntax["config"] = quote(syntax["config"])
        return syntax

class sDNAHulls(object):
    alias = "Convex Hulls"
    desc = "<p>Outputs the convex hulls of network radii used in sDNA Integral analysis."
    category = "Analysis geometry"
    
    def getInputSpec(self):
        return [("input","Input polyline features","FC","Polyline","",True),
                  ("output","Output hull polygon features","OFC",None,"",True),
                  ("start_gs","Start grade separation","Field",("Numeric","input"),"",False),
                  ("end_gs","End grade separation","Field",("Numeric","input"),"",False)]\
                  +radius_options(False)+\
                  [("origins","Origin IDs (leave blank for all)","Text",None,"",False),
                  ("disable","Disable lines (field name or expression)","Text",None,"",False),
                  ("oneway","One way restrictions","Field",("Numeric","input"),"",False),
                  ("advanced","Advanced config","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnaintegral"
        syntax["inputs"] = {"net":args["input"]}
        syntax["outputs"] = {"hulls":args["output"]}
        syntax["config"] = "nobetweenness;start_gs=%(start_gs)s;end_gs=%(end_gs)s;"\
                           "nonetdata;outputhulls;"\
                           "origins=%(origins)s;disable=%(disable)s;oneway=%(oneway)s;"\
                           "%(advanced)s;"%args  + radius_config(args)
        syntax["config"] = quote(syntax["config"])
        return syntax

class sDNANetRadii(object):
    alias = "Network Radii"
    desc = "<p>Outputs the network radii used in sDNA Integral analysis."
    category = "Analysis geometry"
    
    def getInputSpec(self):
        return [("input","Input polyline features","FC","Polyline","",True),
                  ("output","Output net radius multipolyline features","OFC",None,"",True),
                  ("start_gs","Start grade separation","Field",("Numeric","input"),"",False),
                  ("end_gs","End grade separation","Field",("Numeric","input"),"",False)]\
                  +radius_options()+\
                  [("origins","Origin IDs (leave blank for all)","Text",None,"",False),
                  ("disable","Disable lines (field name or expression)","Text",None,"",False),
                  ("oneway","One way restrictions","Field",("Numeric","input"),"",False),
                  ("advanced","Advanced config","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnaintegral"
        syntax["inputs"] = {"net":args["input"]}
        syntax["outputs"] = {"netradii":args["output"]}
        syntax["config"] = "nobetweenness;start_gs=%(start_gs)s;end_gs=%(end_gs)s;"\
                           "nonetdata;outputnetradii;"\
                           "origins=%(origins)s;disable=%(disable)s;oneway=%(oneway)s;"\
                           "%(advanced)s;"%args  + radius_config(args)
        syntax["config"] = quote(syntax["config"])
        return syntax

class sDNAAccessibilityMap(object):
    alias = "Specific Origin Accessibility Maps"
    desc = "<p>Outputs accessibility maps for specific origins."
    category = "Analysis"
    
    def getInputSpec(self):
        return [("input","Input polyline features","FC","Polyline","",True),
                  ("output","Output polyline features","OFC",None,"",True),
                  ("start_gs","Start grade separation","Field",("Numeric","input"),"",False),
                  ("end_gs","End grade separation","Field",("Numeric","input"),"",False),
                  ("origins","Origin IDs (leave blank for all)","Text",None,"",False),
                  metric_dropdown("analmet","Routing and analysis metric"),
                  ("custommetric","Custom metric field","Field",("Numeric","input"),"",False),
                  ("disable","Disable lines (field name or expression)","Text",None,"",False),
                  ("oneway","One way restrictions","Field",("Numeric","input"),"",False),
                  ("advanced","Advanced config","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnaintegral"
        syntax["inputs"] = {"net":args["input"]}
        syntax["outputs"] = {"destinations":args["output"]}
        syntax["config"] = "start_gs=%(start_gs)s;end_gs=%(end_gs)s;"\
                           "metric=%(analmet)s;"\
                           "custommetric=%(custommetric)s;"\
                           "nonetdata;outputdestinations;"\
                           "origins=%(origins)s;disable=%(disable)s;oneway=%(oneway)s;"\
                           "%(advanced)s;"%args
        syntax["config"] = quote(syntax["config"])
        return syntax

class sDNAPrepare(object):
    alias = "Prepare Network"
    desc = \
"""<p>Prepares spatial networks for analysis by checking and optionally repairing various kinds of error.
<p><b>Note that sDNA Prepare Network only provides a small subset of the functions needed for network preparation.</b>  Other free tools, combined with a good understanding of the subject, can fill the gap.  <b>Reading the Network Preparation chapter of the sDNA Manual is strongly advised.</b>
<p>The errors fixed by Prepare Network are:
<ul>
<li><b>endpoint near misses</b> (XY and Z tolerance specify how close a near miss)
<li><b>duplicate lines</b>
<li><b>traffic islands</b> (requires traffic island field set to 0 for no island and 1 for island).  Traffic island lines are straightened; if doing so creates duplicate lines then these are removed.
<li><b>split links</b><i>. Note that fixing split links is no longer necessary as of sDNA 3.0 so this is not done by default</i>
<li><b>isolated systems</b>
</ul>
<p>Optionally, numeric data can be preserved through a prepare operation by providing the desired field names.  
"""
    category = "Preparation"
    
    def getInputSpec(self):
        return [("input","Input polyline features","FC","Polyline","",True),
                  ("output","Output polyline features","OFC",None,"",True),
                  ("start_gs","Start grade separation","Field",("Numeric","input"),"",False),
                  ("end_gs","End grade separation","Field",("Numeric","input"),"",False),
                  ("action","Action","Text",["DETECT","REPAIR"],"REPAIR",True),
                  ("nearmisses","Endpoint near misses","Bool",None,True,True),
                  ("trafficislands","Traffic islands","Bool",None,False,True),
                  ("duplicates","Duplicate polylines","Bool",None,True,True),
                  ("isolated","Isolated systems","Bool",None,True,True),
                  ("splitlinks","Split links","Bool",None,False,True),
                  ("tifield","Traffic island field","Field",("Numeric","input"),"",False),
                  ("preserve_absolute","Absolute data to preserve (numeric field names separated by commas)","Text",None,"",False),
                  ("preserve_unitlength","Unit length data to preserve (numeric field names separated by commas)","Text",None,"",False),
                  ("preserve_text","Text data to preserve (text field names separated by commas)","Text",None,"",False),
                  ("merge_if_identical","Do not merge split links if not identical (field names separated by commas)","Text",None,"",False),
                  ("xytol","Custom XY Tolerance","Text",None,"",False),
                  ("ztol","Custom Z Tolerance","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        boollist = [x for x in ["nearmisses","duplicates","isolated","trafficislands","splitlinks"]
                        if args[x]]
        args["boolstring"] = ";".join(boollist)
        syntax = {}
        syntax["command"] = "sdnaprepare"
        syntax["inputs"] = {"net":args["input"]}
        syntax["outputs"] = {"net":args["output"],"errors":args["output"]}
        syntax["config"] = "start_gs=%(start_gs)s;end_gs=%(end_gs)s;"\
                           "xytol=%(xytol)s;ztol=%(ztol)s;"\
                           "action=%(action)s;"\
                           "%(boolstring)s;"\
                           "island=%(tifield)s;"\
                           "data_absolute=%(preserve_absolute)s;data_unitlength=%(preserve_unitlength)s;data_text=%(preserve_text)s;merge_if_identical=%(merge_if_identical)s"\
                           %args
        syntax["config"] = quote(syntax["config"])
        return syntax

class sDNALineMeasures(object):
    alias = "Individual Line Measures"
    desc = \
"""<p>Outputs connectivity, bearing, euclidean, angular and hybrid metrics for individual links.
<p>Connectivity output is useful for checking errors."""
    category = "Preparation"
        
    def getInputSpec(self):
        return [("input","Input polyline features","FC","Polyline","",True),
                  ("output","Output features","OFC",None,"",True),
                  ("start_gs","Start grade separation","Field",("Numeric","input"),"",False),
                  ("end_gs","End grade separation","Field",("Numeric","input"),"",False),
                  metric_dropdown("analmet","Routing and analysis metric")]\
                  +weighting_options()+\
                  [("custommetric","Custom metric field","Field",("Numeric","input"),"",False),
                  ("zonefiles","Zone table input csv files","MultiInFile","csv","",False),
                  ("advanced","Advanced config","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnaintegral"
        syntax["inputs"] = {"net":args["input"]}
        syntax["outputs"] = {"net":args["output"]}
        syntax["config"] = "linkonly;start_gs=%(start_gs)s;end_gs=%(end_gs)s;"\
                           "metric=%(analmet)s;"\
                           "custommetric=%(custommetric)s;"\
                           "%(advanced)s;"%args\
                           + weighting_config(args)
        syntax["config"] = quote(syntax["config"])
        if args["zonefiles"]!="":
            syntax["inputs"]["tables"]=",".join(args["zonefiles"].split(";"))
        return syntax

class sDNALearn(object):
    alias = "Learn"
    desc = \
"""<p>Uses measured data to calibrate an sDNA model ready for prediction. Proposed models are tested using cross-validation.  The available models are

<ul>
<li>Single best variable - performs bivariate regression of target against all variables and picks single predictor with best cross-validated fit</li>
<li>Multiple variables - Regularized multivariate lassoo regression</li>
<li>All variables - Regularized multivariate ridge regression</li>
</ul>

<p>Optionally, variables to use and transform can be specified using regular expressions (regex).  These follow the Python regex syntax.  The equivalent to a wildcard is 
<pre>.*</pre>
<p>thus for example to test Betweenness variables (from sDNA Integral) over all radii you could specify 
<pre>Bt.*</pre>
<p>This would match Bt1000, Bt2000, Bt300c, etc.
<p>Optionally, the best model can be saved as a model file to be used by sDNA Predict.
<p>Weighting lambda weights data points by y^lambda/y.  Setting to 1 implies unweighted regression.  Setting to around 0.7 can improve GEH statistic.
<p>Regression lambda if set should specify min,max regularization parameter for multivariate regression.
"""
    category = "Calibration"
        
    def getInputSpec(self):
        return [("input","Input features","FC",None,"",True),
                  ("output","Output model file","OutFile","csv","",False),
                  ("resids","Output residual features","OFC",None,"",False),
                  ("target","Target variable","Field",("Numeric","input"),"",True),
                  ("predictors","Predictor variables","MultiField",("Numeric","input"),"",False),
                  ("regex","Predictor variable regex","Text",None,"",False),
                  ("algorithm","Learning algorithm","Text",["Single_best_variable","Multiple_variables","All_variables"],"Single_best_variable",True),
                  ("intercept","Use intercept in multivariate models","Bool",None,False,False),
                  ("bctarget","Box-Cox transform target variable","Bool",None,False,False),
                  ("bcregex","Regex for predictor variables to Box-Cox transform","Text",None,"",False),
                  ("weightlambda","Weighting lambda","Text",None,"1",False),
                  ("nfolds","Number of folds for cross-validation","Text",None,"7",True),
                  ("reps","Number of repetitions for bootstrapping","Text",None,"50",True),
                  ("gehtime","Time interval for measurements (in hours, for computing GEH)","Text",None,"1",True),
                  ("reglambda","Regularization lambda min,max","Text",None,"",False)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnalearn"
        syntax["inputs"] = {"calibfile":args["input"]}
        syntax["outputs"] = {"modelout":args["output"],"resids":args["resids"]}
        syntax["config"] = "--target %(target)s --mode %(algorithm)s --nfolds %(nfolds)s --weightlambda %(weightlambda)s --reps %(reps)s --gehtime %(gehtime)s --bcregex \"%(bcregex)s\""%args
        if args["predictors"]:
            syntax["config"] += " --vars \""+args["predictors"]+"\""
        if args["regex"]:
            syntax["config"] += " --varregex \""+args["regex"]+"\""
        if args["bctarget"]:
            syntax["config"] += " --boxcoxtarget"
        if args["intercept"]:
            syntax["config"] += " --intercept"
        if args["reglambda"].strip() != "":
            syntax["config"] += " --reglambda " + args["reglambda"]
        
        return syntax        
        
class sDNAPredict(object):
    alias = "Predict"
    desc = "<p>Uses a model file created by sDNA Learn to predict unknown data."
    category = "Calibration"
        
    def getInputSpec(self):
        return [("input","Input features","FC",None,"",True),
                  ("output","Output features","OFC",None,"",True),
                  ("predvar","Prediction variable name","Text",None,"prediction",True),
                  ("modelfile","Model file from sDNA Learn","InFile","csv","",True)
                  ]

    def getSyntax(self, args):
        syntax = {}
        syntax["command"] = "sdnapredict"
        syntax["inputs"] = {"infile":args["input"]}
        syntax["outputs"] = {"outfile":args["output"]}
        syntax["config"] = '--predvarname %(predvar)s --modelfile "%(modelfile)s"'%args
        
        return syntax        
        
def get_tools():
    return [sDNAIntegral,sDNASkim,sDNAIntegralFromOD,sDNAGeodesics,sDNAHulls,sDNANetRadii,sDNAAccessibilityMap,sDNAPrepare,sDNALineMeasures,sDNALearn,sDNAPredict]
