import getpass
import logging
import os
import pwd
import time
import shutil
import subprocess

class AlienTokenException(Exception):

    def __init__(self, tokencert: str, tokenkey: str, errorcode: int, detail: str):
        super(AlienTokenException, self).__init__()
        self.__tokencert = tokencert
        self.__tokenkey = tokenkey
        self.__errorcode = errorcode
        self.__detail = detail

    def __str__(self):
        return "(Error {ERR}) Tokenfiles {CERT} and {KEY} invalid ...".format(ERR=self.__errorcode, CERT=self.__tokencert, KEY=self.__tokenkey)

    def get_cert(self) -> str:
        return self.__tokencert

    def get_key(self) -> str:
        return self.__tokenkey

    def get_errorcode(self) -> str:
        return self.__errorcode

    def get_detail(self):
        return self.__detail

class AlienToken:

    def __init__(self, dn: str, issuer: str, begin: time.struct_time, end: time.struct_time):
        self.__dn = dn
        self.__issuer = issuer
        self.__begin = begin
        self.__end = end

    def set_begin(self, begin: time.struct_time):
        self.__begin = begin

    def set_end(self, end: time.struct_time):
        self.__end = end
    
    def set_dn(self, dn: str):
        self.__dn = dn

    def set_issuer(self, issuer: str):
        self.__issuer = issuer

    def get_begin(self) -> time.struct_time:
        return self.__begin

    def get_end(self) -> time.struct_time:
        return self.__end

    def get_dn(self) -> str:
        return self.__dn

    def get_issuer(self) -> str:
        return self.__issuer

def parse_time(token_timestring: str):
    return time.strptime(token_timestring, "%Y-%m-%d %H:%M:%S")

def get_token_info(tokencert: str, tokenkey: str):
    testcmd="export JALIEN_TOKEN_CERT={ALIEN_CERT}; export JALIEN_TOKEN_KEY={ALIEN_KEY}; alien-token-info".format(ALIEN_CERT=tokencert, ALIEN_KEY=tokenkey)
    testres = subprocess.getstatusoutput(testcmd)
    if testres[0] != 0:
        raise AlienTokenException(tokencert, tokenkey, testres[0], testres[1])
    infos = testres[1].split("\n")
    dn = ""
    issuer = ""
    start = None
    end = None
    for en in infos:
        if not ">>>" in en:
            continue
        keyval = en.split(">>>")
        key = keyval[0].lstrip().rstrip()
        value = keyval[1].lstrip().rstrip()
        if key == "DN":
            dn = value
        elif key == "ISSUER":
            issuer = value
        elif key == "BEGIN":
            start = parse_time(value)
        elif key == "EXPIRE":
            end = parse_time(value)
    return AlienToken(dn, issuer, start, end)

def test_alien_token():
    result = {}
    me = getpass.getuser()
    userid = pwd.getpwnam(me).pw_uid
    cluster_tokenrepo = os.path.join("/software", me, "tokens")
    cluster_tokencert = os.path.join(cluster_tokenrepo, "tokencert_%d.pem" %userid)
    cluster_tokenkey = os.path.join(cluster_tokenrepo, "tokenkey_%d.pem" %userid)
    if not os.path.exists(cluster_tokencert) or not os.path.exists(cluster_tokenkey):
        logging.error("Either token certificate or key missing ...")
        return result
    try:
        tokeninfo = get_token_info(cluster_tokencert, cluster_tokenkey)
        now = time.localtime()
        timediff_seconds = time.mktime(tokeninfo.get_end()) - time.mktime(now)
        two_hours = 2 * 60 * 60
        if timediff_seconds > two_hours:
            # accept token if valid > 2 hours
            result = {"cert": cluster_tokencert, "key": cluster_tokenkey}
        return result
    except AlienTokenException as e:
        logging.error(e)
        logging.error("detail: %s", e.get_detail())
        return result

def recreate_token():
    me = getpass.getuser()
    userid = pwd.getpwnam(me).pw_uid
    cluster_tokenrepo = os.path.join("/software", me, "tokens")
    cluster_tokencert = os.path.join(cluster_tokenrepo, "tokencert_%d.pem" %userid)
    cluster_tokenkey = os.path.join(cluster_tokenrepo, "tokenkey_%d.pem" %userid)
    tmp_tokencert = os.path.join("/tmp", "tokencert_%d.pem" %userid)
    tmp_tokenkey = os.path.join("/tmp", "tokenkey_%d.pem" %userid)
    subprocess.call("alien-token-init", shell=True)
    shutil.copyfile(tmp_tokencert, cluster_tokencert)
    shutil.copyfile(tmp_tokenkey, cluster_tokenkey)
    return {"cert": cluster_tokencert, "key": cluster_tokenkey}