#! /usr/bin/env python2

# Author: Christopher Celio
# Date  : 2010 Nov 6
#   
# Python script to run memory hierarhcy ukernels, collect data, and generate
# plots


# change plot size, font sizes for opitmal poster printing
PLOT_POSTER=1

# readjust the Python import path to accept our own common file
import sys
sys.path.append('../common/')
import os

# Put all of the common functions into ccbench, and make run_test.py as small
# as possible. run_test.py should basically only hold the graphing code, since
# that is custom to the benchmark.
import ccbench      
import ccprocstats

APP = "caches"
BASE_DIR="./"
REPORT_DIR=BASE_DIR + "reports/"
PLOT_DIR=BASE_DIR + "plots/"
DEFAULT_REPORT_NAME = "report.txt"
DEFAULT_INPUT_NAME = "inputs.txt"

# These are the variables we are going to parse the input and report files for.
input_variables = (
    "AppSize",
    "NumIterations",
    "RunType"
    )
 
variables = (
    "AppSize",
    "Time",
    "TimeUnits",
    "NumIterations",
    "RunType",
    "NumDataPointsPerSet"
    )
 
# Sometimes we run on remote platforms without access to matplotlib.  In this
# case, don't import the graphing functions and don't run the graphing code. 
# We'll just graph from the report file at a later date on another machine.
NOPLOT = False
try:
    import matplotlib
    matplotlib.use('PDF') # must be called immediately, and before import pylab
                          # sets the back-end for matplotlib
    import matplotlib.pyplot as plt
    import numpy as np
    import pylab
except:
    NOPLOT = True
    print "Failure to import {matplotlib/numpy/pylab}. Graphing turned off."

# pretty-print out the access latency at various levels in the memory hierarchy
# input "size" (in kB) 
# freq is in G-cycles per second
def outputAccessLatency(data, size, memstring, freq):
    closest_idx = 0
    closest_dist = 9999999999 #some max number
    
    for i in range(len(data["AppSize"])):
        # only get data from the random RunType
        if (data["RunType"][i] != "0"):
            continue
        datum = data["AppSize"][i]
        new_dist = abs(size - float(datum))
        if (new_dist <= closest_dist):
            closest_dist = new_dist
            closest_idx = i

    time_ns = data["Time"][closest_idx]
    cycles = float(time_ns) * freq
    # pretty print kB vs MB
    if (float(data["AppSize"][closest_idx]) > 1000):
        print "  %s Access Latency =  %s ns @(%3g MB), %3.2f cycles" % \
            (memstring, data["Time"][closest_idx], float(data["AppSize"][closest_idx])/1024, cycles)   
    else:
        print "  %s Access Latency =  %s ns @(%3g kB), %3.2f cycles" % \
            (memstring, data["Time"][closest_idx], float(data["AppSize"][closest_idx]), cycles)   
    

# 1. Parses input file.
# 2. Runs benchmark (many times) and records results in results file.
# 3. Parses results file for data (can also play back recording using NORUN option).
# 4. Graphs results and outputs to .pdf file.
def main():
    # handle CLI options
    ccbench.controller()  

    #handle default/cli args app
    app_bin = BASE_DIR + APP
    input_filename = BASE_DIR +  DEFAULT_INPUT_NAME
    report_filename = REPORT_DIR + ccbench.getReportFileName(APP, REPORT_DIR)   
   
    # 1. Parse inputs.txt file.
    if (not ccbench.NORUN):
        inputs = ccbench.parseInputFile(input_filename, variables, ccbench.INPUT_TYPE)
        inputs["NumDataPointsPerSet"] = []
        inputs["NumDataPointsPerSet"].append(str(len(inputs["AppSize"])))

        # Build up the arguments list for each invocation of the benchmark.
        # This is done here, instead of in ccbench.py, because this is custom to each app.
        app_args_list = []
        for run_type in inputs["RunType"]: 
            for app_size in inputs["AppSize"]:
                app_args_list.append(str(app_size) + " " \
                    + str(inputs["NumIterations"][0]) + " " +str(run_type))


    # 2. Execute the benchmark and write to the report file.
    if (not ccbench.NORUN):
        ccbench.runBenchmark(app_bin, app_args_list, inputs, input_variables, report_filename)
        
    
    # 3. Extract Data from the report file.
    data = ccbench.readReportFile(report_filename, variables, sort="AppSize")

    
    # 4. Plot the Data
    #print data
    
    if NOPLOT:
        return

    if PLOT_POSTER:
        fig = plt.figure(figsize=(5,3.5))
        font = {#'family' : 'normal',
            #'weight' : 'bold',
            'size'   : 8}
        matplotlib.rc('font', **font)
        fig.subplots_adjust(top=0.94, bottom=0.14, left=0.1, right=0.96,wspace=0, hspace=0)
    else:
        fig = plt.figure(figsize=(9,5.5))
        fig.subplots_adjust(top=0.95, bottom=0.12, left=0.07, right=0.97,wspace=0, hspace=0)
    
    p1 = fig.add_subplot(1,1,1)

    print "Plotting time..."
    num_datapoints = int(data["NumDataPointsPerSet"][0])
 
    # let's convert "appsizearg(#elm)" to "appsize(KB)"
    for i in range(len(data["AppSize"])):
        data["AppSize"][i] = float(data["AppSize"][i]) * 4 / 1024

    for i in range(len(data["Time"])):
        data["Time"][i] = float(data["Time"][i])
    
    for i in range(len(data["AppSize"])/num_datapoints):
        srt_idx = i*num_datapoints
        end_idx = (i+1)*num_datapoints
        p1.plot(
            data["AppSize"][srt_idx:end_idx],
            data["Time"][srt_idx:end_idx],
            linestyle='--',
            marker='.'
            )
    
    p1.set_xscale('log')
    p1.set_yscale('log')
    plt.ylabel(data["TimeUnits"][0])
    plt.xlabel('Array Size')
    plt.ylim((1, 320))
    
    xmin,xmax=plt.xlim()
    plt.xlim((0.5, 1024*64))
    
    # deal with ticks
    xtick_range = [1,2,4,8,16,32,64, 128,256, 512,1024,2048,4096,4096*2, 16384,16384*2,16384*4]
    xtick_names = ['1 kB','2 kB','4 kB','8 kB','16 kB','32 kB','64 kB','128 kB','256 kB','512 kB','1 MB','2 MB','4 MB','8 MB','16 MB','32 MB','64 MB'] #for KB
    ytick_range = [1,2,4,8,16,32,64,128,256] # in ns / iteration
    ytick_names = ['1','2','4','8','16','32','64','128','256']
                     
    if (xmax > 1024*65):
        xtick_range = [1,2,4,8,16,32,64, 128,256, 512,1024,2048,4096,4096*2, 16384,16384*2,16384*4,1024*128]
        xtick_names = ['1 kB','2 kB','4 kB','8 kB','16 kB','32 kB','64 kB','128 kB','256 kB','512 kB','1 MB','2 MB','4 MB','8 MB','16 MB','32 MB','64 MB','128 MB'] #for KB
        plt.xlim((0.5,1024*128))
                     
    if (xmax > 1024*128):
        xtick_range = [1,2,4,8,16,32,64, 128,256, 512,1024,2048,4096,4096*2, 16384,16384*2,16384*4,1024*128,1024*256]
        xtick_names = ['1 kB','2 kB','4 kB','8 kB','16 kB','32 kB','64 kB','128 kB','256 kB','512 kB','1 MB','2 MB','4 MB','8 MB','16 MB','32 MB','64 MB','128 MB','256 MB'] #for KB
        plt.xlim((0.5,1024*256))



    p1.xaxis.set_major_locator( plt.NullLocator() )
    p1.xaxis.set_minor_locator( plt.NullLocator() )
    p1.yaxis.set_major_locator( plt.NullLocator() )
    p1.yaxis.set_minor_locator( plt.NullLocator() )
    plt.xticks(
        xtick_range,
        xtick_names,
        rotation='-30',
        size='small' 
        )
    plt.yticks(ytick_range,ytick_names)
    
    # annotate the graph with pretty cache size lines and info!
    ylow = 160
    yhigh = 200
    ccprocstats.plotCacheSizeLines(plt, p1, ccbench.PROCESSOR, ylow, yhigh)

    # customize figure title and other doodads based on the processor the test was run on
    print " ";
    if (ccbench.PROCESSOR == "merom"):
        outputAccessLatency(data,     8, "L1      ", 2.33);
        outputAccessLatency(data,    64, "L2      ", 2.33);
        outputAccessLatency(data, 16384, "Off-chip", 2.33);
    if (ccbench.PROCESSOR == "ivybridge"):
        outputAccessLatency(data,     8, "L1      ", 2.3);
        outputAccessLatency(data,   128, "L2      ", 2.3);
        outputAccessLatency(data,  3072, "L3      ", 2.3);
        outputAccessLatency(data, 16384, "Off-chip", 2.3);
    elif (ccbench.PROCESSOR == "cuda1"):
        outputAccessLatency(data,     8, "L1      ", 2.4);
        outputAccessLatency(data,    64, "L2      ", 2.4);
        outputAccessLatency(data, 16384, "Off-chip", 2.4);
    elif (ccbench.PROCESSOR == "tegra2"):
        outputAccessLatency(data,     8, "L1      ", 0.4);  #umnown
        outputAccessLatency(data,    64, "L2      ", 0.4);
        outputAccessLatency(data, 16384, "Off-chip", 0.4);
    elif (ccbench.PROCESSOR == "tilera"):
        outputAccessLatency(data,     4, "L1      ", 0.7);
        outputAccessLatency(data,    32, "L2      ", 0.7);
        outputAccessLatency(data,  1024, "L3*     ", 0.7);
        outputAccessLatency(data, 16384, "Off-chip", 0.7);
    elif (ccbench.PROCESSOR == "tilera-l3"):              #unknown
        outputAccessLatency(data,     4, "L1      ", 0.7);
        outputAccessLatency(data,    32, "L2      ", 0.7);
        outputAccessLatency(data,  1024, "L3      ", 0.7);
        outputAccessLatency(data, 16384, "Off-chip", 0.7);
    elif (ccbench.PROCESSOR == "arrandale"):
        outputAccessLatency(data,     8, "L1      ", 3.0); #unknwno
        outputAccessLatency(data,    64, "L2      ", 3.0);
        outputAccessLatency(data,  1024, "L3      ", 3.0);
        outputAccessLatency(data, 16384, "Off-chip", 3.0);
    elif (ccbench.PROCESSOR == "bloomfield"):
        outputAccessLatency(data,     8, "L1      ", 3.4); #unknown
        outputAccessLatency(data,    64, "L2      ", 3.4);
        outputAccessLatency(data,  1024, "L3      ", 3.4);
        outputAccessLatency(data, 16384, "Off-chip", 3.4);
    elif (ccbench.PROCESSOR == "emerald"):
        outputAccessLatency(data,     8, "L1      ", 2.27);
        outputAccessLatency(data,    64, "L2      ", 2.27);
        outputAccessLatency(data,  1024, "L3      ", 2.27);
        outputAccessLatency(data, 16384, "L3      ", 2.27);
        outputAccessLatency(data, 128*1024, "Off-chip", 2.27);
    elif (ccbench.PROCESSOR == "boxboro"): #boxboro.millennium machine (westmere-ex)
        outputAccessLatency(data,     8, "L1      ", 2.27);
        outputAccessLatency(data,    64, "L2      ", 2.27);
        outputAccessLatency(data,  1024, "L3      ", 2.27);
        outputAccessLatency(data, 65536, "Off-chip", 2.27);
        outputAccessLatency(data,262144, "DRAM    ", 2.27);
    elif (ccbench.PROCESSOR == "bridge"): 
        outputAccessLatency(data,     8, "L1      ", 3.4);
        outputAccessLatency(data,    64, "L2      ", 3.4);
        outputAccessLatency(data,  1024, "L3      ", 3.4);
        outputAccessLatency(data,262144, "DRAM    ", 3.4);
    else:
        plt.title("Cache Hierarchy" + r'', fontstyle='italic')
        print "  Unknown processor - cycle count is assuming a 1 GHz clock"
        outputAccessLatency(data,     8, "", 1.0);
        outputAccessLatency(data,    64, "", 1.0);
        outputAccessLatency(data,  1024, "", 1.0);
        outputAccessLatency(data, 16384, "", 1.0);
    print " ";

    if (ccbench.PLOT_FILENAME == "none"):
        filename = PLOT_DIR + ccbench.generatePlotFileName(APP)
    else:
        # Pull out the filename path from the full path.
        # This allows us to pull out the requested filename from the path presented
        # (since we always write reports to the report directory, etc.). However, it
        # allows the user to use tab-completion to specify the exact reportfile he
        # wants to use.
        filename = PLOT_DIR + os.path.basename(ccbench.PLOT_FILENAME)
        filename = os.path.splitext(filename)[0]
        
    plt.savefig(filename)
    print "Used report filename             : " + report_filename 
    print "Finished Plotting, saved as file : " + filename + ".pdf"



                
#This idiom means the below code only runs when executed from the command line
if __name__ == '__main__':
  main()
  #print 'finished with main from CLI'

