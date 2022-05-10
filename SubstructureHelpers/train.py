import subprocess
import logging

class AliTrainDB:

    class UninitializedException(Exception):

        def __init__(self):
            super().__init__()
    
        def __str__(self):
            return "Train database not initialized"

    class TrainNotFoundException(Exception):

        def __init__(self, trainID: int):
            super().__init__()
            self.__trainID = trainID

        def __str__(self):
            return "No train found for ID {}".format(self.__trainID)

        def getTrainID(self) -> int:
            return self.__trainID

    def __init__(self, pwg: str, train: str):
        self.__pwg = pwg
        self.__train = train
        self.__trains = {}
        self.__initialized = False
        self.__build()

    def __build(self):
        trainsraw = subprocess.getstatusoutput("alien_ls /alice/cern.ch/user/a/alitrain/{}/{}".format(self.__pwg, self.__train))
        if trainsraw[0] != 0:
            logging.error("Failed building trains DB for train %s/%s", self.__pwg, self.__train)
        for trainstring in trainsraw[1].split("\n"):
            if 'CryptographyDeprecationWarning' in trainstring or "cryptography" in trainstring:
                continue
            tmpstring = trainstring.replace("/", "").lstrip().rstrip()
            if "_child" in tmpstring:
                tmpstring = tmpstring[0:tmpstring.index("_child")]
            trainID = int(tmpstring.split("_")[0])
            if not trainID in self.__trains.keys():
                logging.debug("{}: Adding ID {}".format(trainID, tmpstring))
                self.__trains[trainID] = tmpstring
        self.__initialized = True

    def getTrainIdentifier(self, trainID: int) -> str:
        if not self.__initialized:
            raise AliTrainDB.UninitializedException()
        if not trainID in self.__trains.keys():
            raise AliTrainDB.TrainNotFoundException(trainID)
        return self.__trains[trainID] 