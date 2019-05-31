import re,os

def getVersion():
    version = None
    versionfile = os.path.dirname(__file__)+os.sep+"sDNA"+os.sep+"sdna_vs2008"+os.sep+"version_template.h"
    for line in (open (versionfile)):
        m = re.match('^const char \*SDNA_VERSION = "(.*)";.*',line)
        if m:
            assert (version==None)
            version = m.group(1)
    return version