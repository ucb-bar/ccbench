#! /usr/bin/env python

# Author: Christopher Celio
# Date  : 2011 June 5
#   
# Python script to run memory hierarhcy ukernels, collect data, and generate
# plots

# change plot size, font sizes for opitmal poster printing
PLOT_POSTER=1
PLOT_PPT=0

# readjust the Python import path to accept our own common file
import sys
sys.path.append('../common/')
import os

# Put all of the common functions into ccbench, and make run_test.py as small
# as possible. run_test.py should basically only hold the graphing code, since
# that is custom to the benchmark.
import ccbench      
import ccprocstats

APP = "cache2cache"
BASE_DIR="./"
REPORT_DIR=BASE_DIR + "reports/"
PLOT_DIR=BASE_DIR + "plots/"
DEFAULT_REPORT_NAME = "report.txt"
DEFAULT_INPUT_NAME = "inputs.txt"


# These are the variables we are going to parse the input and report files for.
input_variables = (
    "AppSize",
    "NumIterations"
    )
 
variables = (
    "AppSize",
    "Output",
    "Units",
    "NumIterations",
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
        for app_size in inputs["AppSize"]:
            app_args_list.append(str(app_size) + " " + str(inputs["NumIterations"][0]))


    # 2. Execute the benchmark and write to the report file.
    if (not ccbench.NORUN):
        ccbench.runBenchmark(app_bin, app_args_list, inputs, input_variables, report_filename)
        
    
    # 3. Extract Data from the report file.
    data = ccbench.readReportFile(report_filename, variables)

    
    # 4. Plot the Data
    #print data
    
    if NOPLOT:
        return

     
    if PLOT_PPT:
        font = {#'family' : 'normal',
            #'weight' : 'bold',
            'size'   : 30}
        matplotlib.rc('font', **font)
        fig.subplots_adjust(top=0.90, bottom=0.25, left=0.18, right=0.97,wspace=0, hspace=0)
        xtick_range = [1,32,256, 4096*2,16384*4]
        xtick_names = ['1 kB','32 kB','256 kB','8 MB','64 MB'] #for KB
    elif PLOT_POSTER:
        fig = plt.figure(figsize=(5,3.5))
        font = {#'family' : 'normal',
            #'weight' : 'bold',
            'size'   : 8}
        matplotlib.rc('font', **font)
        fig.subplots_adjust(top=0.94, bottom=0.14, left=0.1, right=0.96,wspace=0, hspace=0)
    else:
        fig = plt.figure(figsize=(9,5.5))
        fig.subplots_adjust(top=0.95, bottom=0.12, left=0.07, right=0.97,wspace=0, hspace=0)
        fig.subplots_adjust(top=0.95, bottom=0.12, left=0.1, right=0.97,wspace=0, hspace=0)
        

    p1 = fig.add_subplot(1,1,1)

    print "Plotting time..."
    num_datapoints = int(data["NumDataPointsPerSet"][0])
 
    # let's convert "appsizearg(#elm)" to "appsize(KB)"
    for i in range(len(data["AppSize"])):
        data["AppSize"][i] = str(float(data["AppSize"][i]) * 4 / 1024)
    
    
    for i in range(len(data["AppSize"])/num_datapoints):
        srt_idx = i*num_datapoints
        end_idx = (i+1)*num_datapoints
        p1.plot(
            data["AppSize"][srt_idx:end_idx],
            data["Output"][srt_idx:end_idx],
            linestyle='--',
            marker='.'#,
#            linewidth=2
            )
    
    p1.set_xscale('log')
    p1.set_yscale('log')
    plt.ylabel(data["Units"][0])
    plt.xlabel('Array Size')
#    plt.ylim((1, 50000))
    if (ccbench.PROCESSOR == "tilera-l3"):
        plt.ylim((100, 10000))
    elif (ccbench.PROCESSOR == "tilera"):
        plt.ylim((100, 10000))
    else:
        plt.ylim((1000, 100000))

    plt.xlim((0.5, 16384*4))
   
    if not PLOT_PPT:
        # deal with ticks
        xtick_range = [1,2,4,8,16,32,64, 128,256, 512,1024,2048,4096,4096*2, 16384,16384*2,16384*4]
        xtick_names = ['1 kB','2 kB','4 kB','8 kB','16 kB','32 kB','64 kB','128 kB','256 kB','512 kB','1 MB','2 MB','4 MB','8 MB','16 MB','32 MB','64 MB'] #for KB
#    ytick_range = [1,10,100,,1000,10000]
#    ytick_names = ['1','10','100','1e3','10e3']

    p1.xaxis.set_major_locator( plt.NullLocator() )
    p1.xaxis.set_minor_locator( plt.NullLocator() )
#    p1.yaxis.set_major_locator( plt.NullLocator() )
#    p1.yaxis.set_minor_locator( plt.NullLocator() )
    plt.xticks(
        xtick_range,
        xtick_names,
        rotation='-30',
        size='small' 
        )
#    plt.yticks(ytick_range,ytick_names)
    

    (ymin, ymax) = plt.ylim()

    print "ymax=%d log(ymax) = %f, " % (ymax, np.log(ymax))
#    print "log(7) = %f, 2^3 = 8 (%f) " % (np.log(7), np.exp(2))
    yvalue_top = np.exp2((np.log2(ymax) + np.log2(ymin))*0.42)
    yvalue_bot = np.exp2((np.log2(ymax) + np.log2(ymin))*0.40) 
#    yvalue_top = ymax*0.95 #np.exp(np.log(ymax) + np.log(ymin)/(2))
#    yvalue_bot = yvalue_top*0.95 #np.exp(np.log(ymax) + np.log(ymin)/(2)) 
    ccprocstats.plotCacheSizeLines(plt, p1, ccbench.PROCESSOR, yvalue_bot, yvalue_top)

    
    
    if (ccbench.PLOT_FILENAME == "none"):
        filename = ccbench.generatePlotFileName(APP)
    else:
        filename = PLOT_DIR + os.path.basename(ccbench.PLOT_FILENAME)
        filename = os.path.splitext(filename)[0]

    plt.savefig(filename)
    print "Used report filename             : " + report_filename 
    print "Finished Plotting, saved as file : " + filename + ".pdf"
                
                
#This idiom means the below code only runs when executed from the command line
if __name__ == '__main__':
  main()
  #print 'finished with main from CLI'

