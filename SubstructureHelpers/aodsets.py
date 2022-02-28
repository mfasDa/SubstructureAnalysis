#! /usr/bin/env python

def get_current_AODset(dataset: str) -> str:
    aodset = "AOD"
    if dataset.startswith("LHC19d3"):
        aodset = "AOD215"
    elif dataset == "LHC16j5":
        aodset = "AOD200"
    return aodset