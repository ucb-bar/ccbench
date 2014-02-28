#! /usr/bin/env python

# Author: Christopher Celio
# Date  : 2010 Nov 6
#   
# Python script to run memory hierarhcy ukernels, collect data, and generate
# plots


# readjust the Python import path to accept our own common file
import sys
sys.path.append('../common/')

#from matplotlib.font_manager import FontProperties

# Put all of the common functions into ccbench, and make run_test.py as small
# as possible. run_test.py should basically only hold the graphing code, since
# that is custom to the benchmark.
import ccbench      

APP = "mem_interleaving"
BASE_DIR="./"
REPORT_DIR=BASE_DIR + "reports/"
PLOT_DIR=BASE_DIR + "plots/"
DEFAULT_REPORT_NAME = "report.txt"
DEFAULT_INPUT_NAME = "inputs.txt"

# These are the variables we are going to parse the input file for. 
# A bit of ugliness here so we can talk to the generic common.py file.
xinput_variable = "NumRequests"
yinput_variable = "AppSize"
zinput_variable = ""

input_variables = (
    "NumRequests",
    "AppSize",
    "NumIterations"
    )

# These are the variables we are going to parse the report file for.
variables = (
    "NumRequests",
    "AppSize",
    "Time",
    "TimeUnits",
    "Bandwidth",
    "BandwidthUnits",
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


# 1. Parses input file.
# 2. Runs benchmark (many times) and records results in results file.
# 3. Parses results file for data (can also play back recording using NORUN option).
# 4. Graphs results and outputs to .pdf file.
def main():
    ccbench.controller()  # handle CLI options

    #handle default/cli args app
    app_bin = BASE_DIR + APP
    input_filename = BASE_DIR +  DEFAULT_INPUT_NAME
    report_filename = REPORT_DIR + DEFAULT_REPORT_NAME
    
    # 1. Parse inputs.txt file
    if (not ccbench.NORUN):
        inputs = ccbench.parseInputFile(input_filename, input_variables, ccbench.INPUT_TYPE)
        inputs["NumDataPointsPerSet"] = []
        inputs["NumDataPointsPerSet"].append(str(len(inputs["AppSize"])))

        # Build up the arguments list for each invocation of the benchmark.
        # This is done here, instead of in ccbench.py, because this is custom to each app.
        app_args_list = []
        for num_requests in inputs["NumRequests"]:
            for app_size in inputs["AppSize"]:
                    app_args_list.append(str(num_requests) + " " +  str(app_size) \
                        + " " + str(inputs["NumIterations"][0]))
    
    # 2. Execute the benchmark and read the report file
    if (not ccbench.NORUN):
        ccbench.runBenchmark(app_bin, app_args_list, inputs, input_variables, report_filename)
        
    
    # 3. Extract Data
    data = ccbench.readReportFile(report_filename, variables)

    
    # 4. Plot the Data
    #print data
    
    if NOPLOT:
        return

    fig = plt.figure(figsize=(9,5.5))
    p1 = fig.add_subplot(1,1,1)

    print "Plotting time..."
    num_datapoints = int(data["NumDataPointsPerSet"][0])
 
    # let's convert "appsizearg(#elm)" to "appsize(KB)"
    for i in range(len(data["AppSize"])):
        data["AppSize"][i] = str(float(data["AppSize"][i]) * 4 / 1024)
    
    # deal with ticks
    xtick_range = [1,2,4,8,16,32,64,128,256,512,1024,2048,4096,4096*2,16384] #for KB
    xtick_names = ['1 kB','2 kB','4 kB','8 kB','16 kB','32 kB','64 kB','128 kB','256 kB','512 kB','1 MB','2 MB','4 MB','8 MB','16 MB'] #for KB

    xmin,xmax = plt.xlim()
    print "xmax = %d" % (xmax)

#    if (xmax >= 1024*32):
    xtick_range = [1,2,4,8,16,32,64, 128,256, 512,1024,2048,4096,4096*2, 16384,16384*2,16384*4,1024*128]
    xtick_names = ['1 kB','2 kB','4 kB','8 kB','16 kB','32 kB','64 kB','128 kB','256 kB','512 kB','1 MB','2 MB','4 MB','8 MB','16 MB','32 MB','64 MB','128 MB'] #for KB
    plt.xlim((0.5,1024*128))
    
    
    fig.subplots_adjust(top=0.95, bottom=0.12, left=0.07, right=0.90,wspace=0, hspace=0)
         
    for i in range(len(data["AppSize"])/num_datapoints):
        srt_idx = i*num_datapoints
        end_idx = (i+1)*num_datapoints
        p1.plot(
            data["AppSize"][srt_idx:end_idx-1],
#            data["Time"][srt_idx:end_idx-1],
            data["Bandwidth"][srt_idx:end_idx-1],
            linestyle='--',
            marker='.'
            )
    p1.set_xscale('log')

#    plt.ylabel(data["TimeUnits"][0])
    plt.ylabel(data["BandwidthUnits"][0])
    plt.xlabel('Array Size')
    ymin, ymax = plt.ylim()
    plt.ylim((0.0, ymax))
    plt.xlim((0.5, 16384))

    p1.xaxis.set_major_locator( plt.NullLocator() )
    p1.xaxis.set_minor_locator( plt.NullLocator() )
    plt.xticks(
        xtick_range,
        xtick_names,
        rotation='-30',
        size='small' 
        )


    # figure out y location of text labels normally you can use normalize
    # coords, except i want to specify the x location exactly :/
    ylabel = 0
#    for datum in data["Time"]:
    for datum in data["Bandwidth"]:
        if (ylabel < 1.02*float(datum)):
            ylabel = 1.02*float(datum)

    # customize figure title and other doodads based on the processor the test was run on
    if (ccbench.PROCESSOR == "merom"):
        p1.axvline(x=32,   ymin=0.05, ymax=.9, linestyle='--', color='black')
        p1.axvline(x=4196, ymin=0.05, ymax=.9, linestyle='--', color='black')
        plt.title("Intel Core 2 Duo T7600 (Merom)" + r'', fontstyle='italic')
        p1.text(4,       ylabel,"32 kB", horizontalalignment='center')
        p1.text(512,     ylabel,"4 MB", horizontalalignment='center')
        p1.text(8*1024, ylabel,"off-chip", horizontalalignment='center')
    if (ccbench.PROCESSOR == "tegra2"):
        p1.axvline(x=32,   ymin=0.05, ymax=.9, linestyle='--', color='black')
        plt.title("Tegra 2 (ARM Cortex-A9) Dual-core" + r'', fontstyle='italic')
        p1.text(4,       ylabel,"32 kB", horizontalalignment='center')
        p1.text(8*1024, ylabel,"off-chip", horizontalalignment='center')
    elif (ccbench.PROCESSOR == "tilera"):
        plt.title("TilePro64 (Default malloc behavior)" + r'', fontstyle='italic')
        p1.text(4,       ylabel,"8 kB", horizontalalignment='center',verticalalignment='center')
        p1.text(512,     ylabel,"64 MB", horizontalalignment='center',verticalalignment='center')
        p1.text(16*1024, ylabel,"off-chip", horizontalalignment='center',verticalalignment='center')
    elif (ccbench.PROCESSOR == "tilera-l3"):
        plt.title("TilePro64 (using 'Hash-for-Home')" + r'', fontstyle='italic')
        p1.text(4,       ylabel,"8 kB", horizontalalignment='center',verticalalignment='center')
        p1.text(32,      ylabel,"64 kB", horizontalalignment='center',verticalalignment='center')
        p1.text(2048,    ylabel,"4 MB", horizontalalignment='center',verticalalignment='center')
        p1.text(16*1024, ylabel,"off-chip", horizontalalignment='center',verticalalignment='center')
    elif (ccbench.PROCESSOR == "boxboro"):
        plt.title("Intel Xeon E7 -x8xx-series (Westmere-EX), 10 core" + r'', fontstyle='italic')
        p1.axvline(x=32,   ymin=0.05, ymax=.9, linestyle='--', color='black')
        p1.axvline(x=256, ymin=0.05, ymax=.9, linestyle='--', color='black')
        p1.axvline(x=1024*24, ymin=0.05, ymax=.9, linestyle='--', color='black')
        p1.axvline(x=1024*96, ymin=0.05, ymax=.9, linestyle='--', color='black')
        p1.text(8,       1.15e9,"L1 D-Cache", horizontalalignment='center',verticalalignment='center')
        p1.text(8,       1.1e9,"(32 kB)", horizontalalignment='center',verticalalignment='center')
        p1.text(90,      1.15e9,"L2 Cache", horizontalalignment='center',verticalalignment='center')
        p1.text(90,      1.1e9,"(256 kB)", horizontalalignment='center',verticalalignment='center')
        p1.text(1024*9/2,1.15e9,"L3 Cache", horizontalalignment='center',verticalalignment='center')
        p1.text(1024*9/2,1.1e9,"(24 MB)", horizontalalignment='center',verticalalignment='center')
        p1.text(1024*50, 1.15e9,"Socket", horizontalalignment='center',verticalalignment='center')
        p1.text(1024*50, 1.1e9,"(96 MB)", horizontalalignment='center',verticalalignment='center')
        p1.text(1024*228,1.15e9,"Off-chip", horizontalalignment='center',verticalalignment='center')
    else:
        plt.title("Cache Hierarchy" + r'', fontstyle='italic')
                                    
    # legend 
    colors =('b','g','r','c','m','y','k','b','g','r','c','m')
    legend_sz = len(data["NumRequests"])/num_datapoints
    lines = []

    #for i in reversed(range(legend_sz)):
    for i in (range(legend_sz)):
        lines.append(plt.Line2D([0,10], [0,10], linewidth=3, color=colors[legend_sz-1-i]))
         
    args = range(legend_sz)
    for i in (range(legend_sz)):
        args[legend_sz - 1 - i] = data["NumRequests"][((i)*num_datapoints)]


    #legend_font= FontProperties()
    #legend_font.set_size('medium')
    #plt.legend(lines, args, 'best', title="Num of\nRequests", prop=legend_font)
    plt.legend(lines, args, 'best', title="Num of\nRequests", bbox_to_anchor=(1.12, 0.9)) 

 
    if (ccbench.PLOT_FILENAME == "none"):
        filename = APP + "_" + ccbench.PROCESSOR
    else:
        filename = ccbench.PLOT_FILENAME
        
    plt.savefig(PLOT_DIR + filename)
    print "Finished Plotting, saved as file '" + PLOT_DIR + filename + ".pdf'"
                
                
#This idiom means the below code only runs when executed from the command line
if __name__ == '__main__':
  main()
  #print 'finished with main from CLI'

