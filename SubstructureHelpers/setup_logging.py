#! /usr/bin/env python3
import logging

def setup_logging(debug: bool = False):
    loglevel=logging.INFO
    if debug:
        loglevel = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)