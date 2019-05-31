from itertools import izip
import sys

infiles = ["correctout_geom.txt","testout_geom.txt"]
infiles = map(open,infiles)

def listcompare(a,b,depth=-1):
    depth += 1
    if a==b:
        return True
    if type(a)!=list or type(b)!=list:
        return False
    if len(a)!=len(b):
        return False
    print "depth",depth,"length",len(a)
    for n,(i1,i2) in enumerate(zip(a,b)):
        if not listcompare(i1,i2,depth):
            print "mismatch level",depth,"position",n,"of",len(a)
            print i1
            print i2
            return False
    return True

def maxdepth(a):
    if type(a)!=list:
        return 0
    else:
        return max([maxdepth(x) for x in a])+1
                        

for n,(line1,line2) in enumerate(izip(*infiles)):
    if line1==line2:
        continue
    # we have nonmatching line
    list1 = eval(line1)
    list2 = eval(line2)

    if not listcompare(list1,list2):
        print "line",n
        print "depth",maxdepth(list1),maxdepth(list2)
        print
