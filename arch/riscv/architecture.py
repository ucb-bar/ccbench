#! /usr/bin/env python

# Author: Christopher Celio
# Date  : 2014 Feb 21
#
# Hold architecture-specific Python glue in here. In particular, this code
# handles invoking the benchmark binaries on the desired target architecture.


# Pass in a string of the type of processor (micro-architecture)
# 
import sys
sys.path.append('../common/')
import ccbench
import os

def runBenchmark(processor, app_bin, app_args, report_filename):
    
    if (processor == "spike"):
        cmd = "spike pk "
    else:
        emu = os.getenv("emulator", "emulator")
        cmd = emu + " pk "

    print cmd + app_bin + " " + app_args + " >> " + report_filename
    value = ccbench.runBash(cmd + app_bin + " " + app_args + " >> " + report_filename)
    if value != "": print value
