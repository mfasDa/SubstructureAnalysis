#! /usr/bin/env python3

import argparse
import logging
import os
import sys
from SubstructureHelpers.slurm import ScriptWriter

def getNumberOfRuns(filename):
    nruns = 0
    with open(filename, "r") as reader:
        for line in reader:
            if "run" in line:
                # Filter header line
                continue
            nruns += 1
        reader.close()
    return nruns

def find_lists(repo: str, dopPb: bool) -> list:
    runlists = "RawEventCountspPb" if dopPb else "RawEventCounts"
    runlistdir = os.path.join(repo, runlists)
    return sorted([os.path.join(runlistdir, x) for x in os.listdir(runlistdir) if "LHC" in x and not "bad" in x])

def create_job(repo: str, outputbase: str, runsfile: str, partition: str) -> tuple:
    nruns = getNumberOfRuns(runsfile)
    if not nruns:
        logging.error("Runlist %s does not contain any run", runsfile)
        return (0, None)
    runsfilebase = os.path.basename(runsfile)
    period = runsfilebase[:runsfilebase.find(".csv")]
    year = 2000 + int(period[3:5])
    outputdir = os.path.join(outputbase, "{}".format(year), period)
    if not os.path.exists(outputdir):
        os.makedirs(outputdir, 0o755)

    runscript = os.path.join(repo, "EMCALTrigger", "TriggerMask", "runfilter.sh")

    jobscriptname = os.path.join(outputdir, "filter.sh" )
    job = ScriptWriter(jobscriptname)
    job.logging_config("JOB $SLURM_ARRAY_TASK_ID")
    job.comment("Jobscript automatically created - do not modify")
    job.Nodes(1)
    job.Tasks(1)
    job.partition(partition)
    job.Array(1, getNumberOfRuns(runsfile))
    job.jobname("scan_{}".format(period))
    job.logfile(os.path.join(outputdir, "logs", "job%a.log"))
    job.sum("LINEINFILE", ["SLURM_ARRAY_TASK_ID", 1])
    job.define("RUNNUMBER", "$(sed \"${{LINEINFILE}}q;d\" {} | cut -d \",\" -f1)".format(runsfile))
    job.info("Processing run $RUNNUMBER")
    job.process(runscript, [repo, outputdir, "$RUNNUMBER"])
    job.info("Done")
    return (nruns, job)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitfilter.py", "Submitting OCDB Query for trigger mask")
    parser.add_argument("outputdir", metavar="OUTPUTDIR", type=str, help="Output directory")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="short", help="Partition (default: short)")
    parser.add_argument("-d", "--debug", action="store_true", help="Run in debug mode")
    parser.add_argument("--pPb", action="store_true", help="Running over p-Pb 8 TeV runs")
    args = parser.parse_args()
    debugmode = args.debug
    loglevel = logging.INFO
    if debugmode:
        loglevel = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)

    repo = os.getenv("SUBSTRUCTURE_ROOT")
    if not repo and not len(repo):
        logging.error("Substructure repository not initialized")
        sys.exit(1)

    for period in find_lists(repo, args.pPb):
        periodfile = os.path.basename(period)
        periodname = periodfile[:periodfile.find(".csv")]
        logging.info("Submitting for period: ")
        runs, periodjob = create_job(repo, args.outputdir, period, args.partition)
        if not periodjob:
            continue
        if not debugmode:
            jobid = periodjob.submit()
            logging.info("Submitted %s (%d runs) under job ID %d", periodname, runs, jobid)