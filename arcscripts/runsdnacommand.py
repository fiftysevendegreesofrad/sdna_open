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

import os,sys,time,re
from subprocess import PIPE, Popen, STDOUT
try:
    from queue import Queue, Empty
except ImportError:
    from queue import Queue, Empty
from threading import Thread

def map_to_string(map):
    return '"'+";".join((k+"="+v.replace("\\","/") for k,v in map.items()))+'"'
    
# because select.select doesn't work on Windows,
# create threads to call blocking stdout/stderr pipes
# and update progress when polled
class ForwardPipeToProgress:
    def __init__(self,progress,prefix,pipe,outputblank):
        self.outputblanklines=outputblank
        self.unfinishedline=""
        self.prefix=prefix
        self.progress=progress
        self.q = Queue()
        self.regex = re.compile("Progress: ([0-9][0-9]*.?[0-9]*)%")
        self.seen_cr = False
        def enqueue_output(out, queue):
            while True:
                data = out.read(1) # blocks
                if not data:
                    break
                else:
                    queue.put(data)
        t = Thread(target=enqueue_output, args=(pipe, self.q))
        t.daemon = True # thread won't outlive process
        t.start()
    
    def _output(self):
        m = self.regex.match(self.unfinishedline.strip())
        if m:
            self.progress.setPercentage(float(m.group(1)))
        else:
            if self.outputblanklines or self.unfinishedline!="":
                self.progress.setInfo(self.prefix+self.unfinishedline)
        self.unfinishedline=""
    
    def poll(self):
        while not self.q.empty():
            char = str(self.q.get_nowait(),"ascii")
            # treat \r and \r\n as \n
            if char == "\r":
                self.seen_cr = True
                continue
            if char == "\n" or self.seen_cr:
                self._output()
                if char != "\n":
                    self.unfinishedline+=char
                self.seen_cr = False
            else:
                self.unfinishedline+=char
    
    def flush(self):
        self.poll()
        if self.unfinishedline:
            self._output()

 
def runsdnacommand(syntax,sdnapath,progress,pythonexe=None,pythonpath=None):
    # run command in subprocess, copy stdout/stderr back to progress object
    command = ''
    if pythonexe:
        command += '"'+pythonexe+'" '
    command += '"'+sdnapath.strip('"')+os.sep+syntax["command"]+'.py" --im '+map_to_string(syntax["inputs"])+' --om '+map_to_string(syntax["outputs"])+' '+syntax["config"]
    progress.setInfo("Running external command: "+command+"\n")
    
    # fork subprocess
    ON_POSIX = 'posix' in sys.builtin_module_names
    environ = os.environ.copy()
    environ["PYTHONUNBUFFERED"]="1" # equivalent to calling with "python -u"
    if pythonpath:
        environ["PYTHONPATH"]=pythonpath
    
    # MUST create pipes for stdin, out and err because http://bugs.python.org/issue3905
    p = Popen(command, shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE, bufsize=0, close_fds=ON_POSIX, env=environ)
    fout = ForwardPipeToProgress(progress,"",p.stdout,True)
    ferr = ForwardPipeToProgress(progress,"ERR: ",p.stderr,False)
    
    while p.poll() is None:
        fout.poll()
        ferr.poll()
        time.sleep(0.3)
        
    p.stdout.close()
    p.stderr.close()
    p.stdin.close()
    fout.flush()
    ferr.flush()
    p.wait()
    progress.setInfo("External command completed")
    return p.returncode