sDNA bin folder
===============

This folder contains python scripts for running sDNA directly on shapefiles.

The scripts also support running on arc geodatabases, if ArcGIS 10 or later is installed.  This is to facilitate batching analyses, if you don't like the way it's done in Arc.

Add this folder to your system path for ease of use (the sDNA installer should have done this already).



Easy way to use
===============

If you have set the correct windows file associations for python scripts (.py files), then you will be able to call the scripts directly from the command prompt by typing

sdnaprepare.py --help

or

sdnaintegral.py --help

where the --help option will, of course, give you more information on each script respectively.



Hard way to use
===============

If you haven't, can't or don't want to set up file associations, then you can load python explicitly, e.g. (assuming python is on your system path):

python -u sdnaprepare.py --help

(or if it isn't):

c:\path\to\python -u sdnaprepare.py --help

(or if neither python nor the sdna bin folder are on your path)

c:\path\to\python -u c:\path\to\sdna\bin\sdnaprepare.py --help
