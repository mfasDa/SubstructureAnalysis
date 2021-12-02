#! /usr/bin/env python3
import argparse
import logging
import sys
import xml.etree.ElementTree as xmltree


class Run:

    def __init__(self, runnumber: int, INT7: int, EMC7: int, EG1: int, EG2: int, EJ1: int, EJ2: int, DMC7: int, DG1: int, DG2: int, DJ1: int, DJ2: int):
        self.__runnumber = runnumber
        self.__INT7 = INT7 
        self.__EMC7 = EMC7
        self.__EG1 = EG1
        self.__EG2 = EG2
        self.__EJ1 = EJ1
        self.__EJ2 = EJ2
        self.__DMC7 = DMC7
        self.__DG1 = DG1
        self.__DG2 = DG2
        self.__DJ1 = DJ1
        self.__DJ2 = DJ2 

    def get_runnumber(self) -> int:
        return self.__runnumber

    def __cmp__(self, other):
        if isinstance(other, Run):
            otherrun = other.get_runnumber()
            if self.__runnumber == otherrun:
                return 0
            elif self.__runnumber > otherrun:
                return 1
            else:
                return -1
        return 1

    def __lt__(self, other) -> bool:
        if isinstance(other, Run):
            otherrun = other.get_runnumber()
            return self.__runnumber < otherrun
        return False

    def __eq__(self, other) -> bool:
        if isinstance(other, Run):
            otherrun = other.get_runnumber()
            return self.__runnumber == otherrun
        return False

    def to_csv(self) -> str:
        return "{run},{CINT7},{CEMC7},{EG1},{EG2},{EJ1},{EJ2},{CDMC7},{DG1},{DG2},{DJ1},{DJ2}".format(run=self.__runnumber, CINT7=self.__INT7, CEMC7=self.__EMC7, EG1=self.__EG1, EG2=self.__EG2, EJ1=self.__EJ1, EJ2=self.__EJ2, CDMC7=self.__EMC7, DG1=self.__EG1, DG2=self.__EG2, DJ1=self.__EJ1, DJ2=self.__EJ2)

def select(triggerclasses: map):
    result = 0
    for k, v in triggerclasses.items():
        if v > result:
            result = v
    return result

def parse_alias(triggerclass: str) -> str:
    tmpcls = triggerclass[1:]   # Drop starting C
    # first 4 letters are for L0 trigger, if longer select L1
    if len(tmpcls) == 4:
        #trigger is a L0 trigger
        return tmpcls
    else:
        #trigger is a L1 trigger
        return tmpcls[4:]

def parse_xml(xmlfile: str) -> list:
    runlist = []
    decoded = xmltree.parse(xmlfile)
    xmlruns = [x for x in decoded.getroot() if x.tag == "RUN"]
    logging.info("Found %d runs", len(xmlruns))
    for nextrun in xmlruns:
        logging.debug("Parsing next run")
        triggerclasses = {}
        runnumber = 0
        for node in nextrun:
            value = node.text if node.text != None else 0
            key = node.tag
            logging.debug("Next key: %s", key)
            istrigger = False
            if "__" in key:
                key = key[:key.find("__")]
                istrigger = True
            if key == "RUN":
                runnumber = int(value)
            if istrigger:
                triggerclass = key[:key.find("-")]
                logging.debug("Extracted trigger class: %s from %s", triggerclass, key)
                if triggerclass in triggerclasses.keys():
                    triggerclasses[triggerclass][key] = int(value)
                else:
                    triggerclasses[triggerclass] = {key: int(value)}
        if runnumber == 0:
            logging.error("Error parsing next entry, skipping ...")
            continue
        eventcounts = {}
        for trig, classes in triggerclasses.items():
            alias = parse_alias(trig)
            logging.debug("Select %s", alias)
            eventcounts[alias] = select(classes)
        logging.info("Stored %d triggers for run %d", len(eventcounts), runnumber)
        for key in eventcounts.keys():
            logging.debug("Found trigger alias: %s", key)
        runlist.append(Run(runnumber, eventcounts["INT7"], eventcounts["EMC7"], eventcounts["EG1"], eventcounts["EG2"], eventcounts["EJ1"], eventcounts["EJ2"], 
                                      eventcounts["DMC7"], eventcounts["DG1"], eventcounts["DG2"], eventcounts["DJ1"], eventcounts["DJ2"]))
    return runlist


def process_convert(inputfile: str, outputfile: str):
    runs = parse_xml(inputfile)
    with open(outputfile, "w") as csvwriter:
        csvwriter.write("run,CINT7,CEMC7,EG1,EG2,EJ1,EJ2,CDMC7,DG1,DG2,DJ1,DJ2\n")
        for run in sorted(runs):
            csvwriter.write("{}\n".format(run.to_csv()))
        csvwriter.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser("getRawCountsFromLogbookXML.py", description="Converting logbook trigger xml to csv")
    parser.add_argument("-i", "--inputfile", metavar="INPUTFILE", type=str, required=True, help="Input xml file")
    parser.add_argument("-o", "--outputfile", metavar="OUTPUTFILE", type=str, required=True, help="Output csv file")
    parser.add_argument("-d", "--debug", action="store_true", help="Print debug information")
    args = parser.parse_args()

    loglevel = logging.INFO
    if args.debug:
        loglevel = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)

    process_convert(args.inputfile, args.outputfile)