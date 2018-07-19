#! /usr/bin/env python 

from ROOT import TFile, TList, TObject
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
    result = None
    for o in self.__objects:
      if not result:
        result = o
      else:
        result.Add(o)
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
      print("self scaled %s, has content: %s" %(self.name(), "Yes" if self.hascontent() else "No"))
      print("other scaled %s, has content: %s" %(other.name(), "Yes" if other.hascontent() else "No"))
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
    print("Scalehist found")
    # find non-zero bin
    binid = -1
    for i in range(0, xsechist.GetXaxis().GetNbins()):
      if xsechist.GetBinContent(i+1) > 0:
        binid = i+1
        print("Non-0 bin is %d" %(binid-1))
        break
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
      print("found non-0 weight %e" %weight)
      inputobject.Scale(weight)
  return inputobject

def mergeAnalysisTaskLight(outputfile, mergefiles):
  mergeobjects = []
  handles = []
  for m in sorted(mergefiles):
    print("Reading %s" %m)
    reader = TFile.Open(m, "READ")
    for k in reader.GetListOfKeys():
      scaled = ScaleObject(k.GetName(), k.ReadObj().IsA().GetName(), reweightObject(k.ReadObj(), 1.))
      print("scaled %s, has content: %s" %(scaled.name(), "Yes" if scaled.hascontent() else "No"))
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

  writer = TFile.Open(outputfile, "RECREATE")
  for m in mergeobjects:
    merged = m.getMerged().content()
    merged.Write(m.name(), TObject.kSingleKey)
  writer.cd()

  writer.Close()
  for h in handles:
    h.Close()

if __name__ == "__main__":
  mergeAnalysisTaskLight(sys.argv[1], sys.argv[2:])


