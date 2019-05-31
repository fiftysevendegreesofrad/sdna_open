import sys,os
up = os.path.dirname

encoding = sys.getfilesystemencoding()
path = up(unicode(__file__, encoding))
arcscriptsdir = up(up(up(path)))+r"\arcscripts"
if arcscriptsdir not in sys.path:
    sys.path.insert(0,arcscriptsdir)
