from itertools import izip

infiles = ["correctout_geom.txt","testout_geom.txt"]
outfiles = [x+".structured.txt" for x in infiles]

def structure(x,depth=-1):
    depth += 1
    prefix = " "*depth
    if not type(x)==list:
        return prefix+str(x)+"\n"
    else:
        if all([type(i)!=list for i in x]):
            return prefix+",".join(map(str,x))+"\n"
        else:
            retval=prefix+"[\n"
            for i in x:
                retval += structure(i,depth)
            retval+=prefix+"]\n"
            return retval
            
#print structure([[[1,2],[3,4]],[[5,6],[7,8]]])


for infilen,outfilen in zip(infiles,outfiles):
    infile=open(infilen)
    outfile=open(outfilen,"w")
    for line in infile:
        try:
            data = eval(line)
            outfile.write(structure(data))
        except:
            outfile.write(line)
    outfile.close()
