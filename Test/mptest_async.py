#! /usr/bin/env python

from multiprocessing import Pool
import os
import subprocess

class Payload:

    def __init__(self, workdir, command):
        self.__status = 0
        self.__workdir = workdir
        self.__command = command

    def execute(self):
        self.__status = 1
        if not os.path.exists(self.__workdir):
            os.makedirs(self.__workdir)
        os.chdir(self.__workdir)
        subprocess.call(self.__command, shell = True)
        self.__status = 2

    def isqueued(self):
        return True if self.__status == 0 else False

    def isrunning(self):
        return True if self.__status == 1 else False

    def isdone(self):
        return True if self.__status == 2 else False

def workfunction(payload):
    payload.execute()

class TaskRunner:

    def __init__(self, taskID, basedir, pool):
        self.__taskid = taskID
        self.__basedir = basedir
        self.__pool = Pool
    
    def runtests(self):
        tasks = []
        for i in range(0, 20):
            

if __name__ == "__main__":
    pass