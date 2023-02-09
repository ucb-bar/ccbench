#! /usr/bin/env python

# Author: Christopher Celio
# Date  : 2010 Nov 6
#   
# Python script to run memory hierarhcy ukernels, collect data, and generate
# plots


# readjust the Python import path to accept our own common file
import sys
sys.path.append('../common/')

from matplotlib.font_manager import FontProperties

# Put all of the common functions into ccbench, and make run_test.py as small
# as possible. run_test.py should basically only hold the graphing code, since
# that is custom to the benchmark.
import ccbench      

APP = "strided"
BASE_DIR="./"
REPORT_DIR=BASE_DIR + "reports/"
PLOT_DIR=BASE_DIR + "plots/"
DEFAULT_REPORT_NAME = "report.txt"
DEFAULT_INPUT_NAME = "inputs.txt"

# These are the variables we are going to parse the input and report files for.
input_variables = (
    "NumThreads",
    "AppSize",
    "AppStride",
    "NumIterations"
    )
 
variables = (
    "NumThreads",
    "AppSize",
    "AppStride",
    "Time",
    "TimeUnits",
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
    print("Failure to import {matplotlib/numpy/pylab}. Graphing turned off.")


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
    report_filename = REPORT_DIR + DEFAULT_REPORT_NAME
    
    # 1. Parse inputs.txt file.
    if (not ccbench.NORUN):
        inputs = ccbench.parseInputFile(input_filename, variables, ccbench.INPUT_TYPE)
        inputs["NumDataPointsPerSet"] = []
        inputs["NumDataPointsPerSet"].append(str(len(inputs["AppStride"])))

        # Build up the arguments list for each invocation of the benchmark.
        # This is done here, instead of in ccbench.py, because this is custom to each app.
        app_args_list = []
        for app_size in inputs["AppSize"]:
            for app_stride in inputs["AppStride"]:
                app_args_list.append(str(inputs["NumThreads"][0]) + " " +  str(app_size) \
                    + " " + str(app_stride) + " " + str(inputs["NumIterations"][0]) + " ")


    # 2. Execute the benchmark and write to the report file.
    if (not ccbench.NORUN):
        ccbench.runBenchmark(app_bin, app_args_list, inputs, input_variables, report_filename)
        
    
    # 3. Extract Data from the report file.
    data = ccbench.readReportFile(report_filename, variables)

    
    # 4. Plot the Data
    #print data
    
    if NOPLOT:
        return

    fig = plt.figure(figsize=(10,5.5))
    p1 = fig.add_subplot(1,1,1)

    print("Plotting time...")
    num_datapoints = int(data["NumDataPointsPerSet"][0])
 
    # let's convert "appsizearg(#elm)" to "appsize(KB)"
    for i in range(len(data["AppSize"])):
        data["AppSize"][i] = str(float(data["AppSize"][i]) * 4 / 1024)
    
    # convert stride from Word to Bytes
    for i in range(len(data["AppStride"])):
        data["AppStride"][i] = str(float(data["AppStride"][i]) * 4)
    
    
    for i in range(len(data["AppSize"])/num_datapoints):
        srt_idx = i*num_datapoints
        end_idx = (i+1)*num_datapoints
        p1.plot(
            data["AppStride"][srt_idx:end_idx],
            data["Time"][srt_idx:end_idx],
            linestyle='--',
            marker='.'
            )
    
    p1.set_xscale('log')
    plt.ylabel(data["TimeUnits"][0])
    plt.xlabel('Stride Size')
    xmin,ymax=plt.xlim()
    plt.xlim((xmin, 16384*4))
    
    font = {#'family' : 'normal',
    #    #'weight' : 'bold',
        'size'   : 12}
    matplotlib.rc('font', **font)
    
    # deal with ticks
    xtick_range = [4,    16,   64,   256,  1024, 4096, 16386 , 65544, 262176, 1048704, 4194816, 16779264, 67117056] # in Bytes
    xtick_names = ['4B','16B','64B','256B','1KB','4KB','16KB', '64KB', '256KB', '1MB',   '4MB',   '16MB',   '64MB'] 
    ytick_range = [1,2,4,8,16,32]#,64] #,128,256] # in ns / iteration
    ytick_names = ['1','2','4','8','16','32']#,'64'] #,'128','256']

    p1.xaxis.set_major_locator( plt.NullLocator() )
    p1.xaxis.set_minor_locator( plt.NullLocator() )
    plt.xticks(
        xtick_range,
        xtick_names,
        rotation='-30',
        size='small' 
        )
#    p1.yaxis.set_major_locator( plt.NullLocator() )
#    p1.yaxis.set_minor_locator( plt.NullLocator() )
#    plt.yticks(ytick_range,ytick_names)
    fig.subplots_adjust(top=0.95, bottom=0.12, left=0.06, right=0.90,wspace=0, hspace=0)

         
    # legend 
    print("Adding Legend...")
    colors =('b','g','r','c','m','y','k')
    legend_sz = len(data["AppSize"])/num_datapoints
    lines = []

    for i in reversed(list(range(legend_sz))):
        lines.append(plt.Line2D([0,10], [0,10], linewidth=3, color=colors[i % len(colors)]))
     
    args = list(range(legend_sz))
    for i in range(legend_sz):
        args[legend_sz-i-1] = data["AppSize"][(i*num_datapoints)]

    font = FontProperties(size='small');
    plt.legend(lines, args, 'upper right', title="Array \n Size (KB)", prop=font, bbox_to_anchor=(1.12, 0.95) )



    # customize figure title and other doodads based on the processor the test was run on
    if (ccbench.PROCESSOR == "merom"):
        plt.title("Intel Core 2 Duo T7600 (Merom)" + r'', fontstyle='italic')
    if (ccbench.PROCESSOR == "tegra2"):
        plt.title("Tegra 2 (ARM Cortex-A9)" + r'', fontstyle='italic')
    elif (ccbench.PROCESSOR == "tilera"):
        plt.title("TilePro64 (Default malloc behavior)" + r'', fontstyle='italic')
    elif (ccbench.PROCESSOR == "tilera-l3"):
        plt.title("TilePro64 (using 'Hash-for-Home')" + r'', fontstyle='italic')
    else:
        plt.title("Cache Hierarchy" + r'', fontstyle='italic')
        #plt.title("Intel Core i7 640M (Arrandale)" + r'', fontstyle='italic')
        #plt.title("Intel Core i7 975 Extreme Edition" + r'', fontstyle='italic')

    if (ccbench.PLOT_FILENAME == "none"):
        filename = APP + "_" + ccbench.PROCESSOR
    else:
        filename = ccbench.PLOT_FILENAME
        
    plt.savefig(PLOT_DIR + filename)
    print("Finished Plotting, saved as file '" + PLOT_DIR + filename + ".pdf'")
                
                
#This idiom means the below code only runs when executed from the command line
if __name__ == '__main__':
  main()
  #print 'finished with main from CLI'

