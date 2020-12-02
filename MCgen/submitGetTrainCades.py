#! /usr/bin/env python3
import argparse
import os
import sys
import subprocess

class ScriptWriter:

    def __init__(self, scriptname, interpreter):
        self.__writer = open(scriptname, "w")
        interpreterstring = ""
        if interpreter == "bash":
            interpreterstring = "/bin/bash"
        elif interpreter == "python2":
            interpreterstring = "/usr/bin/env python"
        elif interpreter == "python3":
            interpreterstring = "/usr/bin/env python3"
        self.__write("#! {INTERPERTER}".format(INTERPERTER=interpreterstring))
        
    def sbatch(self, instruction):
        self.__write("#SBATCH {INSTRUCTION}".format(INSTRUCTION=instruction))
    
    def instruction(self, instruction):
        self.__write(instruction)

    def __write(self, line):
        self.__writer.write("{LINE}\n".format(LINE=line))

    def close(self):
        self.__writer.close()

def create_jobscript(configdir, outputdir, executable, njobs):
    if not os.path.exists(outputdir):
        os.makedirs(outputdir, 0o755)
    logdir = os.path.join(outputdir, "logs")
    if not os.path.exists(logdir):
        os.makedirs(logdir, 0o755)
    jobscriptname = os.path.join(outputdir, "jobscript.sh")
    scriptwriter = ScriptWriter(jobscriptname, "bash")
    scriptwriter.sbatch("-N 1")
    scriptwriter.sbatch("-n 1")
    scriptwriter.sbatch("--array=1-{NJOBS}".format(NJOBS=njobs))
    scriptwriter.sbatch("--partition=gpu")
    scriptwriter.sbatch("-A birthright")
    scriptwriter.sbatch("--time=02:00:00")
    scriptwriter.sbatch("--cpus-per-task=1")
    scriptwriter.sbatch("-J gentrain")
    scriptwriter.sbatch("--mem-per-cpu 2G")
    scriptwriter.sbatch("-o {LOGDIR}/joboutput_%a.log".format(LOGDIR=logdir))
    scriptwriter.instruction("PROCID=$SLURM_JOB_ID")
    scriptwriter.instruction("CHUNKID=$SLURM_ARRAY_TASK_ID")
    scriptwriter.instruction("module load PE-gnu singularity")
    singularity_env = "singularity exec -B /nfs/home:/nfs/home -B /lustre:/lustre /home/mfasel_alice/mfasel_cc7_alice.simg"
    envscript = "/home/mfasel_alice/alice_setenv"
    runcmd = "{EXECUTABLE} {CONFIGDIR} {OUTPUTDIR} $PROCID $CHUNKID {ENVSCRIPT}".format(EXECUTABLE=executable, CONFIGDIR=configdir, OUTPUTDIR=outputdir, ENVSCRIPT=envscript)
    scriptwriter.instruction("{SINGULARITY} {RUNCMD}".format(SINGULARITY=singularity_env, RUNCMD=runcmd))
    scriptwriter.close()
    return jobscriptname

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitGenTrainLocal.py", "Local submitter for MC gen train")
    parser.add_argument("outputdir", metavar="OUTPUTDIR", type=str, help="Output location")
    parser.add_argument("-n", "--njobs", metavar="NJOBS", type=int, default=100, help="Number of jobs")
    parser.add_argument("-c", "--configdir", metavar="CONFIGDIR", type=str, default=os.getcwd(), help="Directory with legotrain files")
    args = parser.parse_args()

    sourcedir = os.path.abspath(os.path.dirname(sys.argv[0]))
    executable = os.path.join(sourcedir, "runGenTrainContainer.sh")
    outputdir = os.path.abspath(args.outputdir)
    jobscript = create_jobscript(args.configdir, outputdir, executable, args.njobs)
    print("Submitting script {JOBSCRIPT}".format(JOBSCRIPT=jobscript))
    subprocess.call("sbatch {JOBSCRIPT}".format(JOBSCRIPT=jobscript), shell=True)