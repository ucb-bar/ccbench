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

def runBenchmark(processor, app_bin, app_args, report_filename):
    PCI_ARGS = "--pci-resume --tile 8x7"
    if processor == "tilera-l3":
        MONITOR_COMMON_ARGS = "--batch-mode --env LD_CACHE_HASH=all " \
        + "--mkdir /opt/test --cd /opt/test --upload " + app_bin + " " + app_bin \
        + " -- " +app_bin + " " + app_args
    else:
        MONITOR_COMMON_ARGS = "--batch-mode " \
        + "--mkdir /opt/test --cd /opt/test --upload " + app_bin + " " + app_bin \
        + " -- " + app_bin + " " + app_args
    cmd = "/opt/tile64pro/bin/tile-monitor" + " " \
        + PCI_ARGS + " " +  MONITOR_COMMON_ARGS
    data = runBash("source ~/.bash_profile; " + cmd)
    print "echo \"" + data + "\" >> " + report_filename
    value = runBash("echo \"" + data + "\" >> " + report_filename)
    if value != "": print value
