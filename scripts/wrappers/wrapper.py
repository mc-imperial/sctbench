#! /usr/bin/env python

import sys
import subprocess

if __name__ == "__main__":
    for arg in sys.argv:
        if arg.startswith("-O"):
            sys.argv.remove(arg)
            break
    status = subprocess.call(sys.argv[1:])
    sys.exit(status)

