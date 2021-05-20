import logging
import subprocess

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