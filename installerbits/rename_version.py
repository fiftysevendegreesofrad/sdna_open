import os,re,sys,_parentdir,shutil
from getSdnaVersion import getVersion

installfile,docdir,outputdir = sys.argv[1:4]

print installfile
print docdir
print outputdir

version = getVersion()

filename_friendly_version = re.sub("\.","_",version)

outfilename = "%s/sDNA_open_setup_win_v%s.msi"%(outputdir,filename_friendly_version)

if (os.path.exists(outfilename)):
    os.unlink(outfilename)
    
os.rename(installfile,outfilename)

outdocname = "%s/sDNA_open_manual_v%s"%(outputdir,filename_friendly_version)

shutil.rmtree(outdocname,ignore_errors=True)
shutil.copytree(docdir,outputdir+os.sep+outdocname)

redirfile = open(outputdir+"/latest.html","w")
redirfile.write("""
<html><head>
<meta http-equiv="refresh" content="0; url=%s" />
</head>
<body>
If you are not redirected please click <a href="%s">here</a>.
</body>
</html>
"""%(outdocname,outdocname))
redirfile.close()