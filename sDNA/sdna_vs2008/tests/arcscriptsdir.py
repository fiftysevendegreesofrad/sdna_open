import sys,os
up = os.path.dirname

path = up(__file__)
arcscriptsdir = up(up(up(path)))+r"\arcscripts"
if arcscriptsdir not in sys.path:
    sys.path.insert(0,arcscriptsdir)
