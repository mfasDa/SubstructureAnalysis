#! /usr/bin/env python

import argparse
import os
import subprocess
import sys

repository = os.path.abspath(os.path.dirname(sys.argv[0]))

def createJobscript(jobscriptname, repo, mcsample, trainrun, outputdir):
    if not os.path.exists(os.path.dirname(jobscriptname)):
        os.makedirs(os.path.dirname(jobscriptname), 0755)
    with open(jobscriptname, 'w') as writer:
        writer.write("#! /bin/bash\n")
        writer.write("#PBS -o %s/transfer.log\n", outputdir)
        writer.write("#PBS -j oe\n")
        writer.write("if [ ! -d %s ]; then mkdir -p %s; fi\n", outputdir, outputdir)
        writer.write("if [ -f %s/run ]; then rm %s/run; fi\n", outputdir, outputdir)
        writer.write("if [ -f %s/done ]; then rm %s/done; fi\n", outputdir, outputdir)
        writer.write("export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib:/usr/lib/x86_64-linux-gnu\n")
        writer.write("eval `/usr/local/bin/alienv -w /home/markus/alice/sw/ --no-refresh printenv AliRoot/latest-ali-work-root6`\n")
        writer.write("/usr/local/bin/alienv -w /home/markus/alice/sw/ --no-refresh  list\n")
        writer.write("touch %s/run\n", outputdir)
        writer.write("%s/copyFromGrid.py %s %s %s\n", repo, mcsample, trainrun, outputdir)
        writer.write("rm %s/run\n", outputdir)
        writer.write("touch %s/done\n", outputdir)
        writer.write("rm -rf %s\n", jobscriptname)
        writer.close()

def submitCopy(mcsample, trainrun, outputdir):
    jobscriptname = "%s/jobscript.sh" %outputdir
    createJobscript(jobscriptname, repository, mcsample, trainrun, outputdir)
    subprocess.call(["qsub", jobscriptname])

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="submitCopuMC.py", description="Run copy of MC pt-hard production as PBS job")
    parser.add_argument("sample", metavar = "SAMPLE", help="Path in alien to the sample base directory")
    parser.add_argument("trainrun", metavar = "TRAINRUN", help = "Full name of the train run (i. e. PWGJE/Jets_EMC_pp_MC/xxxx)")
    parser.add_argument("outputpath", metavar = "OUTPUTPATH", help = "Local directory where to write the output to")
    args = parser.parse_args()
    submitCopy(args.sample, args.trainrun, args.outputpath)