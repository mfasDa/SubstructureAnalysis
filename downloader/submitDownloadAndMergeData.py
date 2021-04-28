#! /usr/bin/env python3

import argparse
import getpass
import logging
import os
import pwd
import shutil
import subprocess
import sys
import time

def submit(command: str, jobname: str, logfile: str, partition: str = "short", numnodes: int = 1, numtasks: int = 1, jobarray = None, dependency=0) -> int:
    submitcmd = "sbatch -N {NUMNODES} -n {NUMTASKS} --partition={PARTITION}".format(NUMNODES=numnodes, NUMTASKS=numtasks, PARTITION=partition)
    if jobarray:
        submitcmd += " --array={ARRAYMIN}-{ARRAYMAX}".format(ARRAYMIN=jobarray[0], ARRAYMAX=jobarray[1])
    if dependency > 0:
        submitcmd += " -d {DEP}".format(DEP=dependency)
    submitcmd += " -J {JOBNAME} -o {LOGFILE} {COMMAND}".format(JOBNAME=jobname, LOGFILE=logfile, COMMAND=command)
    print("Submitcmd: {}".format(submitcmd))
    submitResult = subprocess.run(submitcmd, shell=True, stdout=subprocess.PIPE)
    sout = submitResult.stdout.decode("utf-8")
    toks = sout.split(" ")
    jobid = int(toks[len(toks)-1])
    return jobid

class AlienToken:

    def __init__(self, dn: str, issuer: str, begin: time.struct_time, end: time.struct_time):
        self.__dn = dn
        self.__issuer = issuer
        self.__begin = begin
        self.__end = end

    def set_begin(self, begin: time.struct_time):
        self.__begin = begin

    def set_end(self, end: time.struct_time):
        self.__end = end
    
    def set_dn(self, dn: str):
        self.__dn = dn

    def set_issuer(self, issuer: str):
        self.__issuer = issuer

    def get_begin(self) -> time.struct_time:
        return self.__begin

    def get_end(self) -> time.struct_time:
        return self.__end

    def get_dn(self) -> str:
        return self.__dn

    def get_issuer(self) -> str:
        return self.__issuer

def parse_time(token_timestring: str):
    return time.strptime(token_timestring, "%Y-%m-%d %H:%M:%S")

def get_token_info(tokencert: str, tokenkey: str):
    testcmd="export JALIEN_TOKEN_CERT={ALIEN_CERT}; export JALIEN_TOKEN_KEY={ALIEN_KEY}; alien-token-info".format(ALIEN_CERT=tokencert, ALIEN_KEY=tokenkey)
    testres = subprocess.getstatusoutput(testcmd)
    if testres[0] != 0:
        logging.error("Tokenfiles %s and %s invalid ...", tokencert, tokenkey)
        return None
    infos = testres[1].split("\n")
    dn = ""
    issuer = ""
    start = None
    end = None
    for en in infos:
        keyval = en.split(">>>")
        key = keyval[0].lstrip().rstrip()
        value = keyval[1].lstrip().rstrip()
        if key == "DN":
            dn = value
        elif key == "ISSUER":
            issuer = value
        elif key == "BEGIN":
            start = parse_time(value)
        elif key == "EXPIRE":
            end = parse_time(value)
    return AlienToken(dn, issuer, start, end)

def test_alien_token():
    result = {}
    me = getpass.getuser()
    userid = pwd.getpwnam(me).pw_uid
    cluster_tokenrepo = os.path.join("/software", me, "tokens")
    cluster_tokencert = os.path.join(cluster_tokenrepo, "tokencert_%d.pem" %userid)
    cluster_tokenkey = os.path.join(cluster_tokenrepo, "tokenkey_%d.pem" %userid)
    if not os.path.exists(cluster_tokencert) or not os.path.exists(cluster_tokenkey):
        logging.error("Either token certificate or key missing ...")
        return result
    tokeninfo = get_token_info(cluster_tokencert, cluster_tokenkey)
    now = time.localtime()
    timediff_seconds = time.mktime(tokeninfo.get_end()) - time.mktime(now)
    two_hours = 2 * 60 * 60
    if timediff_seconds > two_hours:
        # accept token if valid > 2 hours
        result = {"cert": cluster_tokencert, "key": cluster_tokenkey}
    return result

def recreate_token():
    me = getpass.getuser()
    userid = pwd.getpwnam(me).pw_uid
    cluster_tokenrepo = os.path.join("/software", me, "tokens")
    cluster_tokencert = os.path.join(cluster_tokenrepo, "tokencert_%d.pem" %userid)
    cluster_tokenkey = os.path.join(cluster_tokenrepo, "tokenkey_%d.pem" %userid)
    tmp_tokencert = os.path.join("/tmp", "tokencert_%d.pem" %userid)
    tmp_tokenkey = os.path.join("/tmp", "tokenkey_%d.pem" %userid)
    subprocess.call("alien-token-init", shell=True)
    shutil.copyfile(tmp_tokencert, cluster_tokencert)
    shutil.copyfile(tmp_tokenkey, cluster_tokenkey)
    return {"cert": cluster_tokencert, "key": cluster_tokenkey}

class LaunchHandler:

    def __init__(self, repo: str, outputbase: str , trainrun: str):
        self.__repo = repo
        self.__outputbase = outputbase
        self.__trainrun = trainrun
        self.__partitionDownload = "short"
        self.__tokens = {"cert": None, "key": None}

    def set_partition_for_download(self, partition: str):
        if not partition in ["long", "short", "vip", "loginOnly"]:
            return
        self.__partitionDownload = partition

    def set_token(self, cert: str, key: str):
        self.__tokens["cert"] = cert
        self.__tokens["key"] = key

    def submit(self, year: int):
        cert = self.__tokens["cert"]
        key = self.__tokens["key"]
        if not key or not cert:
            logging.error("Alien token not provided - cannot download ...")
            return None
        executable = os.path.join(self.__repo, "runDownloadAndMergeDataBatch.sh.")
        jobname = "down_{YEAR}".format(YEAR=year)
        logfile = os.path.join(self.__outputbase, "download.log")
        downloadcmd = "{EXE} {DOWNLOADREPO} {OUTPUTDIR} {YEAR} {TRAINRUN} {ALIEN_CERT} {ALIEN_KEY}".format(EXE=executable, DOWNLOADREPO = self.__repo, OUTPUTDIR=self.__outputbase, YEAR=year, TRAINRUN=self.__trainrun, ALIEN_CERT=cert, ALIEN_KEY=key)
        jobid = submit(command=downloadcmd, jobname=jobname, logfile=logfile, partition=self.__partitionDownload, numnodes=1, numtasks=1)
        return jobid

if __name__ == "__main__":
    currentbase = os.getcwd()
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    logging.basicConfig(format="[%(levelname)s]: %(message)s", level=logging.INFO)
    parser = argparse.ArgumentParser("submitDownloadAndMergeMC.py", description="submitter for download and merge")
    parser.add_argument("-o", "--outputdir", metavar="VARIATION", type=str, default=currentbase, help="Output directory (default: current directory)")
    parser.add_argument("-y", "--year", metavar="YEAR", type=int,required=True, help="Year of the sample")
    parser.add_argument("-t", "--trainrun", metavar="TRAINRUN", type=str, required=True, help="Train run")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="short", help="Partition for download")
    args = parser.parse_args()

    tokens = test_alien_token()
    if not len(tokens):
        logging.info("No valid tokens found, recreating ...")
        tokens = recreate_token()
    if not len(tokens):
        logging.error("Failed generating tokens ...")
        sys.exit(1)
    cert = tokens["cert"]
    key = tokens["key"]

    handler = LaunchHandler(repo=repo, outputbase=args.outputdir, trainrun=args.trainrun)
    handler.set_token(cert, key)
    handler.set_partition_for_download(args.partition)
    handler.submit(args.year)