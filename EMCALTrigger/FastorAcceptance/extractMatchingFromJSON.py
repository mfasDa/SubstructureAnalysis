#! /usr/bin/env python

import os
import simplejson as jsonhandler

from ROOT import TCanvas
from ROOT import TGraph
from ROOT import TH1F
from ROOT import TLegend
from ROOT import kBlack, kOrange, kMagenta, kViolet, kGreen, kBlue, kRed

graphicsobjects = []

class Style:
  def __init__(self, color, marker):
    self.__color = color
    self.__marker = marker

  def set_style(self, graph):
    graph.SetMarkerColor(self.__color)
    graph.SetMarkerStyle(self.__marker)
    graph.SetLineColor(self.__color)

def getRuns(inputdir):
  return sorted([int(r) for r in os.listdir(inputdir) if str(r).isdigit() and os.path.exists(os.path.join(inputdir, r, "maskcomparison.json"))])

def get_from_json(inputfile):
  result = None
  with open(inputfile, 'r') as reader:
    jsonstring = ""
    for line in reader:
      jsonstring += line
    result = jsonhandler.loads(jsonstring)
    reader.close()
  return result

def save_canvas(plot, basename):
  formats = ["eps", "pdf", "png", "jpg", "gif"]
  for f in formats:
    plot.SaveAs("%s.%s" %(basename, f))

def extractMatchingFromJSON(inputdir):
  trendAll = TGraph()
  trendOCDBL0 = TGraph()
  trendOCDBL1 = TGraph()
  trendL0L1 = TGraph()
  trendOCDB = TGraph()
  trendL0 = TGraph()
  trendL1 = TGraph() 

  graphicsobjects.append(trendAll)
  graphicsobjects.append(trendOCDBL0)
  graphicsobjects.append(trendOCDBL1)
  graphicsobjects.append(trendL0L1)
  graphicsobjects.append(trendOCDB)
  graphicsobjects.append(trendL0)
  graphicsobjects.append(trendL1)

  npoint = 0
  runmin = 100000000000
  runmax = 0
  for r in getRuns(inputdir):
    if r < runmin:
      runmin = r
    if r > runmax:
      runmax = r
    data = get_from_json(os.path.join(inputdir, "%09d" %r, "maskcomparison.json"))
    sumdead = float(data["sum"])
    trendAll.SetPoint(npoint, r, float(data["all"])/sumdead)
    trendOCDBL0.SetPoint(npoint, r, float(data["ocdbl0"])/sumdead)
    trendOCDBL1.SetPoint(npoint, r, float(data["ocdbl1"])/sumdead)
    trendL0L1.SetPoint(npoint, r, float(data["l0l1"])/sumdead)
    trendOCDB.SetPoint(npoint, r, float(data["ocdb"])/sumdead)
    trendL0.SetPoint(npoint, r, float(data["l0"])/sumdead)
    trendL1.SetPoint(npoint, r, float(data["l1"])/sumdead)
    npoint += 1

  plot = TCanvas("trend", "Trend mask comparison", 800, 600)
  plot.SetRightMargin(0.25)
  graphicsobjects.append(plot)
  
  frame = TH1F("frame", "; run; fraction dead", runmax - runmin, runmin, runmax)
  frame.SetDirectory(None)
  frame.SetStats(False)
  frame.GetYaxis().SetRangeUser(0., 1.)
  frame.Draw("axis")
  graphicsobjects.append(frame)

  leg = TLegend(0.75, 0.6, 0.99, 0.89)
  leg.SetBorderSize(0)
  leg.SetFillStyle(0)
  leg.SetTextFont(42)
  leg.Draw()
  graphicsobjects.append(leg)

  styleAll = Style(kBlack, 25)
  styleAll.set_style(trendAll)
  trendAll.Draw("epsame")
  leg.AddEntry(trendAll, "ALL", "lp")
  
  styleOCDBL0 = Style(kOrange, 26)
  styleOCDBL0.set_style(trendOCDBL0)
  trendOCDBL0.Draw("epsame")
  leg.AddEntry(trendOCDBL0, "OCDB+L0", "lp")

  styleOCDBL1 = Style(kMagenta, 27)
  styleOCDBL1.set_style(trendOCDBL1)
  trendOCDBL1.Draw("epsame")
  leg.AddEntry(trendOCDBL1, "OCDB+L1", "lp")

  styleL0L1 = Style(kViolet, 28)
  styleL0L1.set_style(trendL0L1)
  trendL0L1.Draw("epsame")
  leg.AddEntry(trendL0L1, "L0+L1", "lp")

  styleOCDB = Style(kRed, 29)
  styleOCDB.set_style(trendOCDB)
  trendOCDB.Draw("epsame")
  leg.AddEntry(trendOCDB, "Only OCDB", "lp")

  styleL0 = Style(kGreen, 30)
  styleL0.set_style(trendL0)
  trendL0.Draw("epsame")
  leg.AddEntry(trendL0, "Only L0", "lp")

  styleL1 = Style(kBlue, 31)
  styleL1.set_style(trendL1)
  trendL1.Draw("epsame")
  leg.AddEntry(trendL1, "Only L1", "lp")

  plot.Update()
  save_canvas(plot, "trendFracMaskOverlap")