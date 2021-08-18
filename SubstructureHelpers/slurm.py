import logging
import os
import subprocess

class ScriptWriter:

    class WorkdirNotSetException(Exception):

        def __init__(self):
            super().__init__(self)

        def __str__(self):
            return "Working directory needs to be defined"

    def __init__(self, filename):
        self.__name = filename
        self.__fileio = open(filename, "w")
        self.__writeline("#! /bin/bash")
        self.__workdirSet = False

    def __del__(self):
        if not self.__fileio.closed:
            self.__fileio.close()

    def __writeline(self, line: str):
        self.__fileio.write("{}\n".format(line))

    def sbatch(self, setting: str):
        self.__writeline("#SBATCH {}".format(setting))
	
    def comment(self, comment: str):
        self.__writeline("# {}".format(comment))

    def printout(self, printout: str):
        self.__writeline("echo {}".format(printout))
	
    def instruction(self, instruction: str):
        self.__writeline(instruction)
    
    def jobname(self, jobname: str):
        self.sbatch("-J {}".format(jobname))

    def logfile(self, logfile: str):
        abslogfile = os.path.abspath(logfile)
        logdir = os.path.dirname(abslogfile)
        if not os.path.exists(logdir):
            os.makedirs(logdir, 0o755)
        self.sbatch("-o {}".format(abslogfile))

    def partition(self, partition: str):
        self.sbatch("--partition={}".format(partition))

    def Nodes(self, nodes: int):
        self.sbatch("-N {}".format(nodes))

    def Tasks(self, tasks: int):
        self.sbatch("-n {}".format(tasks))

    def Array(self, minid: int, maxid: int):
        self.sbatch("--array={}-{}".format(minid, maxid))

    def dependency(self, dependency: int):
        self.sbatch("--dependency={}".format(dependency))

    def workdir(self, directory):
        self.__writeline("export WORKDIR={}".format(directory))
        if not os.path.exists(directory):
            self.__writeline("if [ ! -d $WORKDIR ]; then mkdir -p $WORKDIR; fi")
        self.__writeline("cd $WORKDIR")
        self.__workdirSet = True

    def cd(self, directory):
        self.__writeline("cd {}".format(directory))

    def rmdir(self, directory):
        self.__writeline("rm -rf {}".format(directory))

    def setenv(self, variable: str, value: str):
        self.__writeline("export {}={}".format(variable, value))
    
    def define(self, variable: str, value: str):
        self.__writeline("{}={}".format(variable, value))

    def sum(self, variable: str, values: list):
        cmd="{}=$((".format(variable)
        first = True
        for value in values:
            if not first:
                cmd += "+"
            else:
                first = False
            cmd += "{}".format(value)
        cmd +="))"
        self.__writeline(cmd)

    def modules(self, modules: list):
        for mod in modules:
            self.__writeline("module load {}".format(mod))

    def alienv(self, packages: list):
        self.define("ALIENV", "`which alienv`")
        instruction = "eval `$ALIENV --no-refresh load"
        for package in packages:
            instruction += " {}".format(package)
        instruction += "`"
        self.instruction(instruction)
        self.instruction("eval `$ALIENV list`")

    def process(self, executable: str, args: list = [], logfile: str = ""):
        cmd = executable
        for arg in args:
            cmd += " {}".format(arg)
        if len(logfile):
            cmd += " {}".format(logfile)
        self.__writeline(cmd)

    def stage_in(self, files: list):
        if not self.__workdirSet:
            raise ScriptWriter.WorkdirNotSetException
        for fl in files:
            self.__writeline("cp {} $WORKDIR/".format(fl))

    def stage_out(self, outputdir: str, files: list):
        if not self.__workdirSet:
            raise ScriptWriter.WorkdirNotSetException
        for fl in files:
            self.__writeline("cp $WORKDIR/{} {}".format(fl, outputdir))

    def remove_workdir(self):
        if not self.__workdirSet:
            raise ScriptWriter.WorkdirNotSetException
        self.cd("/")
        self.rmdir("$WORKDIR")

    def submit(self) -> int:
        if not self.__fileio.closed:
            self.__fileio.close()
        return submit_script(self.__name)
        

def submit_script(jobscript: str):
    submitResult = subprocess.run(["sbatch", jobscript], stdout=subprocess.PIPE)
    sout = submitResult.stdout.decode("utf-8")
    toks = sout.split(" ")
    jobid = int(toks[len(toks)-1])
    return jobid

def submit(command: str, jobname: str, logfile: str, partition: str = "short", numnodes: int = 1, numtasks: int = 1, jobarray = None, dependency=0) -> int:
    submitcmd = "sbatch -N {NUMNODES} -n {NUMTASKS} --partition={PARTITION}".format(NUMNODES=numnodes, NUMTASKS=numtasks, PARTITION=partition)
    if jobarray:
        submitcmd += " --array={ARRAYMIN}-{ARRAYMAX}".format(ARRAYMIN=jobarray[0], ARRAYMAX=jobarray[1])
    if dependency > 0:
        submitcmd += " -d {DEP}".format(DEP=dependency)
    submitcmd += " -J {JOBNAME} -o {LOGFILE} {COMMAND}".format(JOBNAME=jobname, LOGFILE=logfile, COMMAND=command)
    logging.debug("Submitcmd: {}".format(submitcmd))
    submitResult = subprocess.run(submitcmd, shell=True, stdout=subprocess.PIPE)
    sout = submitResult.stdout.decode("utf-8")
    toks = sout.split(" ")
    jobid = int(toks[len(toks)-1])
    return jobid