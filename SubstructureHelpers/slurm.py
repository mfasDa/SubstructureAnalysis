import logging
import os
import subprocess

from SubstructureHelpers.cluster import get_cluster, get_default_partition, get_fast_partition, is_valid_partition, PartitionException

class ScriptWriter:

    DEBUG=0
    INFO=1
    WARNING=2
    ERROR=3

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
        self.__logevel = ScriptWriter.INFO
        self.__logtag = ""

    def __del__(self):
        if not self.__fileio.closed:
            self.__fileio.close()

    def __writeline(self, line: str):
        self.__fileio.write("{}\n".format(line))

    def __getLevelName(self, loglevel: int):
        if loglevel == ScriptWriter.DEBUG:
            return "DEBUG"
        elif loglevel == ScriptWriter.INFO:
            return "INFO"
        elif loglevel == ScriptWriter.WARNING:
            return "WARNING"
        elif loglevel == ScriptWriter.ERROR:
            return "ERROR"

    def logging_config(self, tag):
        self.__logtag = tag

    def sbatch(self, setting: str):
        self.__writeline("#SBATCH {}".format(setting))
	
    def comment(self, comment: str):
        self.__writeline("# {}".format(comment))

    def printout(self, printout: str):
        self.__writeline("echo {}".format(printout))

    def log(self, level: int, message: str):
        if level >= self.__logevel:
            fullmsg = "[{}]".format(self.__getLevelName(level))
            if len(self.__logtag):
                fullmsg += "[{}]: ".format(self.__logtag)
            fullmsg += message
            self.printout(fullmsg)

    def debug(self, message: str):
        self.log(ScriptWriter.DEBUG, message)
    
    def info(self, message: str):
        self.log(ScriptWriter.INFO, message)
	
    def warning(self, message: str):
        self.log(ScriptWriter.WARNING, message)

    def error(self, message: str):
        self.log(ScriptWriter.ERROR, message)

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
                cmd += " + "
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
        self.instruction("eval $ALIENV list")

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

def submit(command: str, jobname: str, logfile: str, partition: str = "short", numnodes: int = 1, numtasks: int = 1, jobarray = None, dependency: int = 0, maxtime: str ="8:00:00") -> int:
    cluster = get_cluster()
    submitcmd = "sbatch"
    if cluster == "CADES":
        submitcmd += " -A birthright" 
    if not is_valid_partition(partition, cluster):
        raise PartitionException(partition, cluster)
    requestpartition = partition
    if requestpartition == "default":
        requestpartition = get_default_partition(cluster)
    elif requestpartition == "fast":
        requestpartition = get_fast_partition(cluster)
    submitcmd +=" -N {NUMNODES} -c {NUMTASKS} --partition={PARTITION}".format(NUMNODES=numnodes, NUMTASKS=numtasks, PARTITION=requestpartition)
    if jobarray:
        submitcmd += " --array={ARRAYMIN}-{ARRAYMAX}".format(ARRAYMIN=jobarray[0], ARRAYMAX=jobarray[1])
    if dependency > 0:
        submitcmd += " -d {DEP}".format(DEP=dependency)
    submitcmd += " -J {JOBNAME} -o {LOGFILE}".format(JOBNAME=jobname, LOGFILE=logfile)
    runcommand = command
    if cluster == "CADES":
        submitcmd += " --mem=4G --time={MAXTIME}".format(MAXTIME=maxtime)
        # On CADES scripts must run inside a container
        runcommand = "{} {}".format(os.path.join(os.getenv("SUBSTRUCTURE_ROOT"), "cadescontainerwrapper.sh"), command)
    submitcmd += " {COMMAND}".format(COMMAND=runcommand)
    logging.debug("Submitcmd: {}".format(submitcmd))
    submitResult = subprocess.run(submitcmd, shell=True, stdout=subprocess.PIPE)
    sout = submitResult.stdout.decode("utf-8")
    toks = sout.split(" ")
    jobid = int(toks[len(toks)-1])
    return jobid

def submit_dependencies(command: str, jobname: str, logfile: str, partition: str = "short", numnodes: int = 1, numtasks: int = 1, jobarray = None, dependency: list = [], dependency_mode: str = "afterany", maxtime: str ="8:00:00") -> int:
    cluster = get_cluster()
    submitcmd = "sbatch"
    if cluster == "CADES":
        submitcmd += " -A birthright" 
    if not is_valid_partition(partition, cluster):
        raise PartitionException(partition, cluster)
    requestpartition = partition
    if requestpartition == "default":
        requestpartition = get_default_partition(cluster)
    elif requestpartition == "fast":
        requestpartition = get_fast_partition(cluster)
    submitcmd +=" -N {NUMNODES} -c {NUMTASKS} --partition={PARTITION}".format(NUMNODES=numnodes, NUMTASKS=numtasks, PARTITION=requestpartition)
    if jobarray:
        submitcmd += " --array={ARRAYMIN}-{ARRAYMAX}".format(ARRAYMIN=jobarray[0], ARRAYMAX=jobarray[1])
    if len(dependency):
        dependencystring = dependency_mode
        for dep in dependency:
            dependencystring += ":{}".format(dep)
        submitcmd += " --dependency={DEP}".format(DEP=dependencystring)
    submitcmd += " -J {JOBNAME} -o {LOGFILE}".format(JOBNAME=jobname, LOGFILE=logfile)
    runcommand = command
    if cluster == "CADES":
        submitcmd += " --mem=4G --time={MAXTIME}".format(MAXTIME=maxtime)
        # On CADES scripts must run inside a container
        runcommand = "{} {}".format(os.path.join(os.getenv("SUBSTRUCTURE_ROOT"), "cadescontainerwrapper.sh"), command)
    submitcmd += " {COMMAND}".format(COMMAND=runcommand)
    logging.debug("Submitcmd: {}".format(submitcmd))
    submitResult = subprocess.run(submitcmd, shell=True, stdout=subprocess.PIPE)
    sout = submitResult.stdout.decode("utf-8")
    toks = sout.split(" ")
    jobid = int(toks[len(toks)-1])
    return jobid