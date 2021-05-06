#! /usr/bin/env python3
import os

if __name__ == "__main__":
    basedir = os.getcwd()
    samples = os.listdir(basedir)

    for sample in sorted(samples):
        print("Doing sample {}".format(sample))
        sampledir = os.path.join(basedir, sample)
        for ipth in range(1, 21):
            ptharddir = os.path.join(sampledir, "%02d" %ipth)
            message = "%02d: " %ipth
            first = True
            for run in sorted(os.listdir(ptharddir)):
                if not first:
                    message += ", "
                else:
                    first = False
                message += run
            print(message)