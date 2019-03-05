#! /usr/bin/env python 

from ROOT import TFile, TList, TObject
import argparse
import logging
import sys

class MergeObject():
  
  def __init__(self, name):
    self.__name = name
    self.__objects = []

  def name(self):
    return self.__name

  def addobject(self,scaleobject):
    self.__objects.append(scaleobject)

  def getMerged(self):
    logging.debug("Start mergeing %s" %self.__name)    
    result = None
    for o in self.__objects:
      if not result:
        result = o
      else:
        logging.debug("addding next")    
        result.Add(o)
    logging.debug("Merging done")
    return result


class ScaleObject:
  
  def __init__(self, outputname, outputtype, content):
    self.__outputname = outputname
    self.__outputtype = outputtype
    self.__content = content
  
  def hascontent(self):
    return self.__content != None

  def content(self):
    return self.__content

  def name(self):
    return self.__outputname

  def Add(self, other):
    if self.__outputtype == "TDirectoryFile":
      othercontent = other.content()
      logging.info("self scaled %s, has content: %s" %(self.name(), "Yes" if self.hascontent() else "No"))
      logging.info("other scaled %s, has content: %s" %(other.name(), "Yes" if other.hascontent() else "No"))
      for c in self.__content.GetListOfKeys():
        for o in othercontent.GetListOfKeys():
          if c.GetName() == o.GetName():
            others = TList()
            others.append(o.ReadObj())
            c.ReadObj().Merge(others)
    else:
      others = TList()
      others.append(other.content())
      self.__content.Merge(others)

def extractWeight(inputobject):
  if inputobject.InheritsFrom("TCollection"):
    xsechist = inputobject.FindObject("fHistXsectionExternalFile")
    ntrialshist = inputobject.FindObject("fHistTrialsExternalFile")
    if not xsechist or not ntrialshist:
      return 1.
    logging.info("Scalehist found")
    # find non-zero bin
    binid = -1
    for i in range(0, xsechist.GetXaxis().GetNbins()):
      if xsechist.GetBinContent(i+1) > 0:
        binid = i+1
        logging.info("Non-0 bin is %d" %(binid-1))
        #break
    if binid == -1:
      return 1.
    return xsechist.GetBinContent(binid) / ntrialshist.GetBinContent(binid)
  else:
    return 1.

def reweightObject(inputobject, weight):
  # do not reweight scaling hists
  objname = str(inputobject.GetName())
  if "Xsection" in objname or "ExternalFile" in objname or inputobject.InheritsFrom("TProfile"):
    return inputobject
  if weight == 1.:
    weight = extractWeight(inputobject)
  if inputobject.InheritsFrom("TCollection"):
    for o in inputobject:
      reweightObject(o, weight)
  else:
    if weight != 1.:
      logging.info("found non-0 weight %e" %weight)
      inputobject.Scale(weight)
  return inputobject

def mergeAnalysisTaskLight(outputfile, mergefiles):
  mergeobjects = []
  handles = []
  for m in sorted(int(b.rsplit('/')[1]) for b in mergefiles):
    filename = [x for x in mergefiles if ("%02d" %m) in x][0]
    logging.info("Reading %s" %filename)
    reader = TFile.Open(filename, "READ")
    for k in reader.GetListOfKeys():
      scaled = ScaleObject(k.GetName(), k.ReadObj().IsA().GetName(), reweightObject(k.ReadObj(), 1.))
      logging.info("scaled %s, has content: %s" %(scaled.name(), "Yes" if scaled.hascontent() else "No"))
      mymerge = None
      for o in mergeobjects:
        if o.name() == k.GetName():
          mymerge = o
      if mymerge:
        mymerge.addobject(scaled)
      else:
        mymerge = MergeObject(k.GetName())
        mymerge.addobject(scaled)
        mergeobjects.append(mymerge)
    handles.append(reader)

  logging.debug("start writing to file")
  writer = TFile.Open(outputfile, "RECREATE")
  logging.debug("output file open")
  for m in mergeobjects:
    merged = m.getMerged().content()
    logging.debug("writing %s", m.name())
    merged.Write(m.name(), TObject.kSingleKey)
  writer.cd()

  logging.debug("closing output file")
  writer.Close()
  logging.debug("closing input handles")
  for h in handles:
    h.Close()

if __name__ == "__main__":
  parser = argparse.ArgumentParser(prog = "mergeAnalysisTaskLight.py", description = "Merge and scale output from AnalysisTaskLight")
  parser.add_argument("outputfile", metavar = "OUTPUTFILE", help = "Resulting output file")
  parser.add_argument('inputfiles', metavar='INPUTFILES', type=str, nargs='+', help='List of input files')
  parser.add_argument("-d", "--debug", action = "store_true",  help = "Run with increased debug level")
  args = parser.parse_args()
  loglevel=logging.INFO
  if args.debug:
    loglevel = logging.DEBUG
  logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)
  mergeAnalysisTaskLight(args.outputfile, args.inputfiles)


