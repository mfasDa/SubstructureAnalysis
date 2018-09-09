#! /usr/bin/env python

from multiprocessing import Process
import os

def processrunner(processID, basedir):
    testoutputdir = os.path.join(basedir, "test%d" %processID)
    if not os.path.exists(testoutputdir):
        os.makedirs(testoutputdir)
    os.chdir(testoutputdir)
    with open("testfile%d.txt" %p, 'w') as outwriter:
        outwriter.write("this is a test from process %d\n" %processID)
        outwriter.close()

if __name__ == "__main__":
    basedir = os.getcwd()
    processes = []
    for p in range(0, 10):
        myprocess = Process(target=processrunner, args=(p, basedir))
        myprocess.start()
    for p in processes:
        p.join()