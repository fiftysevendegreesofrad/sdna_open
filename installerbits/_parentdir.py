import sys,os

try:
    unicode #type: ignore
except NameError:
    unicode = str 

encoding = sys.getfilesystemencoding()
path = os.path.dirname(unicode(__file__, encoding))
parentdir = os.path.dirname(path)
if parentdir not in sys.path:
    sys.path.insert(0,parentdir)
