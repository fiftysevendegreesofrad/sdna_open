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

from runcalculation import runcalculation
import sdna_environment,sdnaexception

import optparse,sys
from optparse import OptionParser

def commandline_integral_prepare(command):
    if command=="sdnaintegral":
        example='python -u sdnaintegral.py -i my_infile -o my_outfile "radii=400,800,n;metric=ANGULAR;cont;nohull;start_gs=sgs;end_gs=egs"\n'
    elif command=="sdnaprepare":
        example='python -u sdnaprepare.py -i my_infile -o my_outfile "action=repair;nearmisses;isolated;start_gs=sgs;end_gs=egs"\n'
    desc = 'Examples:\n'\
           + example\
           + "GDB usage requires ArcGIS to be installed, Shapefile usage doesn't."
    
    op = OptionParser("usage: %prog [options] [config_string]",epilog= desc)

    op.add_option("-i","--infile",dest="infilename",
                  help="Input shapefile or gdb (gdb requires ArcGIS)",metavar="FILE")
    op.add_option("-o","--outfile",dest="outfilename",
                  help="Output shapefile or gdb",metavar="FILE")
    op.add_option("--im","--infilemap",dest="infilemap",
                  help="Mapping of config names to inputs",metavar="FILE")
    op.add_option("--om","--outfilemap",dest="outfilemap",
                  help="Mapping of config names to outputs",metavar="FILE")
    op.add_option("--dll",dest="dll",help=optparse.SUPPRESS_HELP,default="")

    (options,args) = op.parse_args()

    if len(args)==0:
        config_string=""
    elif len(args)==1:
        config_string = args[0]
    else:
        op.error("Trailing arguments on command line")
        
    def complain_if_missing(thing,name):
        if thing==None:
            op.error(name+" not specified")
    
    using_maps = False
    if options.infilemap or options.outfilemap:
        using_maps = True
        if options.infilename or options.outfilename:
            op.error("Can't use in/out filenames and file maps together")
        complain_if_missing(options.infilemap,"input file map")
        complain_if_missing(options.outfilemap,"output file map")
    else:
        complain_if_missing(options.infilename,"input filename")
        complain_if_missing(options.outfilename,"output filename")

    def to_map(s):
        m = {}
        for pair in s.split(";"):
            try:
                key,val = pair.split("=")
            except ValueError:
                op.error("Bad map specifier: "+s)
            m[key.strip()]=val.strip()
        return m
        
    if using_maps:
        infilemap = to_map(options.infilemap)
        outfilemap = to_map(options.outfilemap)
        is_gdb = [sdna_environment.is_gdb(x) for x in infilemap.values()+outfilemap.values()]
        try:
            tolerance_source = infilemap["net"]
        except KeyError:
            tolerance_source = infilemap.values()[0]
    else:
        is_gdb = [sdna_environment.is_gdb(x) for x in [options.infilename,options.outfilename]]
        tolerance_source = options.infilename
        
    if any(is_gdb):
        env = sdna_environment.SdnaArcpyEnvironment(tolerance_source)
        from arc_utils import get_tolerance,get_z_tolerance
        tol = get_tolerance(tolerance_source)
        ztol = get_z_tolerance(tolerance_source)
        config_string += ";arcxytol=%f;arcztol=%f;"%(tol,ztol)
    else:
        env = sdna_environment.SdnaShapefileEnvironment(tolerance_source)

    class DummyOutputMap:
        '''Returns output files based on basename + outputname'''
        def __init__(self,outputbase):
            self.outputbase = outputbase
        def __getitem__(self,outputname):
            if outputname=="net":
                return self.outputbase
            else:
                return self.outputbase+"_"+outputname
                
    if not using_maps:
        infilemap = {"net":options.infilename}
        outfilemap = DummyOutputMap(env.get_out_base(options.outfilename))
    
    try:
        runcalculation(env,
                    command,
                    config_string,
                    infilemap,
                    outfilemap,
                    dll = options.dll)
    
    except sdnaexception.SDNAException as e:
        print "ERROR: "+e.message
        sys.exit(1)
