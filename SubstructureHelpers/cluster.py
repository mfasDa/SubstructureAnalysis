#! /usr/bin/env python

import socket

class PartitionException(Exception):

    def __init__(self, partition: str, cluster: str):
        self.__partition = partition
        self.__cluster = cluster

    def __str__(self):
        return "Not a valid partition for cluster {}: {}".format(self.__cluster, self.__partition)

    def get_cluster(self) -> str:
        return self.__cluster

    def get_partition(self):
        return self.__partition

class UnknownClusterException(Exception):

    def __init__(self, cluster: str):
        self.__cluster = cluster

    def __str__(self):
        return "Cluster not known: {}".format(self.__cluster)

    def get_cluster(self) -> str:
        return self.__cluster

class SubmissionHostNotSupportedException(Exception):

    def __init__(self, hostname: str):
        self.__hostname = hostname

    def __str__(self):
        return "No cluster configured for host: {}".format(self.__hostname) 

    def get_hostname(self) -> str:
        return self.__hostname

def get_cluster():
    hostname = socket.gethostname()
    if "or" in hostname:
        return "CADES"
    elif "pc" in hostname:
        return "B587"
    raise SubmissionHostNotSupportedException(hostname)

def get_default_partition(cluster: str):
    if cluster == "CADES":
        return "high_mem_cd"
    elif cluster == "B587":
        return "long"
    raise UnknownClusterException(cluster)

def get_fast_partition(cluster: str):
    if cluster == "CADES":
        return "high_mem_cd"
    elif cluster == "B587":
        return "short"
    raise UnknownClusterException(cluster)

def is_valid_partition(partition: str, cluster: str) -> bool:
    defaultpartitions = ["default", "fast"]
    if not partition in defaultpartitions:
        if cluster == "":
            if not partition in ["long", "short", "vip", "loginOnly"]:
                return False
        elif cluster == "CADES":
            if not partition in ["high_mem_cd", "gpu"]:
               return False 
    return True