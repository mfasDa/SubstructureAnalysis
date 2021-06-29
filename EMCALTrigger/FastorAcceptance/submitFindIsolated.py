#! /usr/bin/env python3
import os
import sys

from SubstructureHelpers.slurm import submit

class LaunchHandler:

    def __init__(self, outputdir: str, year: int):
        self.__repo = os.getenv("SUBSTRUCTURE_ROOT")
        self.__outputdir = outputdir
        self.__year = year

    def read_runlists(self) -> list:
        yeartag = "LHC{}".format(self.__year % 2000)
        runlists = [x for x in os.listdir(os.path.join(self.__repo, "runlists_EMCAL")) if yeartag in x]
        result = []
        for runlist in runlists:
            print("Reading {}".format(runlist))
            with open(os.path.join(os.path.join(self.__repo, "runlists_EMCAL", runlist)), "r") as runreader:
                for line in runreader:
                    if len(line):
                        for token in line.split(","):
                            runstring = token.rstrip().lstrip()
                            if len(runstring):
                                result.append(runstring)
                runreader.close()
        return result

    def launch(self):
        print("launching jobs for {}".format(self.__year))
        executable = os.path.join(self.__repo, "EMCALTrigger", "FastorAcceptance", "runFindIsolated.sh")
        commonoutputdir = os.path.join(self.__outputdir, "%d" %self.__year)
        if not os.path.exists(commonoutputdir):
            os.makedirs(commonoutputdir, 0o755)
        logdir = os.path.join(commonoutputdir, "logs")
        if not os.path.exists(logdir):
            os.makedirs(logdir, 0o755)
        runlist = self.read_runlists()
        for run in runlist:
            logfile = os.path.join(logdir, "filter_{}".format(run)) 
            jobname = "{}_{}".format(self.__year, run)
            cmd = "{} {} {} {}".format(executable, self.__repo, commonoutputdir, run)
            submit(cmd, jobname, logfile)

if __name__ == "__main__":
    outputdir = sys.argv[1]
    for year in [2016, 2017, 2018]:
        handler = LaunchHandler(outputdir, year)
        handler.launch()