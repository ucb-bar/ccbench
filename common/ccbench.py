#! /usr/bin/env python

# Author: Christopher Celio
# Date  : 2011 May 15
#   
# All common Python routines needed by the run_tests.py go in here.  In
# particular, command-line argument parsing, input file parsing, running the
# benchmarks, storing results to an output file, and parsing the output file
# for graphing all goes in here.
#
# The actual graphing code goes elsewhere, since that gets tailored to the
# individual benchmark.


from subprocess import Popen # now we can make system calls
from subprocess import PIPE  # now we can make system calls
import optparse    # parse CLI options
import os

from datetime import datetime 


# This function takes Bash commands and returns them
def runBash(cmd):
    p = Popen(cmd, shell=True, stdout=PIPE)
    out = p.stdout.read().strip()
    return out #This is the stdout from the shell command
 
                                       
# This function takes in a line from a file, and a search string, and returns 
# the string between the nearest [,] delineators that follow the str
#   thus,       "Cycles=[1234]" returns "1234"
def parseValueFromLine(line, str):
    value = line[line.find(str) + len(str) :]
    srt_idx = value.find("[") + 1
    end_idx = value.find("]")
    value = value[srt_idx : end_idx]
    return value.strip()
                                      

# This function takes in a line from a file, and a search string, and returns 
# the an numerical array between the nearest [,] delineators that follow the str
#   thus,       "Cycles=[1,2,3,4]" returns "[1,2,3,4]" (the list, not the str)
#
# also supports functions such as [2:10,+2] which returns "[2,4,6,8,10]"
def parseIntArrayFromLine(line, str):
    arr = []
    line = line[line.find(str) + len(str) :]
    srt_idx = line.find("[") + 1
    end_idx = line.find("]")
    line = line[srt_idx : end_idx] # 1,2,3,4

    colon_idx = line.find(":")
    
    if colon_idx != -1:
        # run function generator
        # (this code should be re-factored to scale to more operators...)
        
        fcn_idx = line.find("+")
        if fcn_idx != -1: 
            start_num   = int(line[0:colon_idx])
            end_num     = int(line[colon_idx+1:fcn_idx-1])
            inc_num     = int(line[fcn_idx+1:])
            
            new_num = start_num
            
            while (new_num <= end_num):
                arr.append(new_num)
                new_num += inc_num
        
        else:
            fcn_idx = line.find("*")
            if fcn_idx == -1: print "Unsupported Operator... let's pretend it's \"*\"..."
            start_num   = int(line[0:colon_idx])
            end_num     = int(line[colon_idx+1:fcn_idx-1])
            inc_num     = int(line[fcn_idx+1:])
            
            new_num = start_num
            
            while (new_num <= end_num):
                arr.append(new_num)
                new_num *= inc_num

    else:
        # regular parsing... just read out the numbers
        com_idx = line.find(",")

        while com_idx != -1:
            arr.append(int(line[: com_idx]))
            line = line[com_idx+1:]
            com_idx = line.find(",")
        
        #handle last number
        arr.append(int(line))
    
    return arr


# Given a 3d dictionary indexed by
#   data[report_key][variable_key] = array [] of strings (assume length of 1)
#   returns int array [] for use in plotting that uses a single variable_key
#   from all reports
def getDataIntArrayFromDict(data_dict, variable_key):
    return_array = []
    for i,report_key in zip(range(len(data_dict)), data_dict.keys()):
        return_array.append(int(data_dict[report_key][variable_key][0]))
    return return_array


# Take an array and pretty-print a string
# Intended to re-create the input file information from the arrays
def arrayToStr(array):
    if isinstance(array,int):
        return str(array)
    else:
        len(array)
        string =""
        for i in range(len(array)):
            if i != (len(array))-1:
                string += (str(array[i]) + ", ")
            else:
                string += (str(array[i]))
        return string
 

 
#Function to control option parsing in Python
def controller():
    global VERBOSE
    global NORUN
    global INPUT_TYPE
    global REPORT_FILENAME
    global PLOT_FILENAME
    global NOPLOT
    # select which processor we're graphing (sets the title and other incidentals and sometimes invoke options).
    global PROCESSOR        
    # select which processor architecture we're targetting (sets the
    # compiler/build options, as well as how to run the binary on the target machine).
    # Essentially, ARCHITECTURE is the ISA and PROCESSOR is the micro-architecture.
    global ARCHITECTURE
    
    #create instance of Option Parser Module, included in Standard Library
    p = optparse.OptionParser(description='CLI Controller for memory system u-kernels',
                  prog='./run_test.py',
                  version='cc-ukernel 0.6',
                  usage= '%prog [option]')
    p.add_option('--norun','-n', action="store_true",
                  help='Skips running ukernels (plot data from existing results.txt file). Useful for tweaking the plots without rerunning the benchmark.',
                  default = False)
    p.add_option('--input','-i', dest="input_type",
                  help='Set the input-type to use (e.g., \'test\' or \'small\'), which is parsed from the input.txt file.',
                  default = "test")
    p.add_option('--processor','-p', dest="processor",
                  help='Denotes the processor we are graphing, which sets the title of the plot, sets special annotations, etc. See the source-code to add a new processor.',
                  default = "unknown")
    p.add_option('--architecture','-a', dest="architecture",
                  help='Denotes the architecture (ISA) we are going to measure, which sets the compiler/build options, and how to invoke the binary on the desired target machine.',
                  default = "riscv")
    p.add_option('--outfilename','-o', dest="outfilename",
                  help='Filename for the generated output plot. The file is always placed in the \'plots\' directory. If directory name is provided as part of the \'outfilename\', it will be automatically stripped out (allows you to easily specify an existing filename).',
                  default = "none")
    p.add_option('--reportfilename','-r', dest="reportfilename",
                  help='Filename to use for the report file. Data from runs are stored in this file. Also, plots are generated from the data in this file. The file is always placed in the \'reports\' directory. If the directory name is provided as part of the \'reportfilename\', it will be automatically stripped out (allows you to easily specify the existing report you want to use).',
                  default = "none")
    p.add_option('--noplot','-g', action="store_true",
                  help='Skips plotting ukernels (runs ukernels and saves to results.txt file).',
                  default = False)
    p.add_option('--verbose', '-v', action = 'store_true',
                  help='prints verbosely (currently unused)',
                  default = False)

    #option Handling passes correct parameter to runBash 
    options, arguments = p.parse_args()
    NORUN          = options.norun
    INPUT_TYPE     = options.input_type
    REPORT_FILENAME= options.reportfilename
    PLOT_FILENAME  = options.outfilename
    VERBOSE        = options.verbose
    NOPLOT         = options.noplot
    PROCESSOR      = options.processor
    ARCHITECTURE   = options.architecture


# 1. Parse inputs file
def parseInputFile(input_filename, variables, input_type):

    input_file = open(input_filename).readlines() 
        
    inputs = {}
        
    # scan for input_type first
    found_input_type = False
    # mark when we're finished reading params from input_type
    input_finished = False

    for line in input_file:
            
        #already captured the inputs we wanted
        if input_finished:
            continue

        #ignore commented out lines (first char is '#')
        idx = (line.strip()).find("#")
        if idx == 0:
            continue
            
        #handle input_type shenanigans... <pistol whip>
        if found_input_type == False:
            idx = line.find("[")  
            if (idx == 0) and (INPUT_TYPE in line):
                found_input_type = True
            else:
                continue
        else:
            idx = line.find("[")  
            if (idx == 0):
                input_finished = True
                continue


        for variable in variables:
            # (parse for "variable=[1,2,3,4]")
            if variable in line:
                inputs[variable] = parseIntArrayFromLine(line, variable)
                continue

    return inputs
                
         

# app_args is a list of strings, where each string corresponds to a single run to the application binary
def runBenchmark(app_bin, app_args_list, inputs, input_variables, report_filename):
     
    # import the architecture-specific functions
    import sys
    sys.path.append('../arch/' + ARCHITECTURE)
    import architecture

    # We typically graph each data set as sweeping the array size
    # so here we go using it as the "number of datapoints per set".
    # I'm sure there is a better way to do this abstractly
    # but the issue is how can the graphing stuff know how many
    # data points exist by just looking at the report file.
    num_dp_per_set   = inputs["NumDataPointsPerSet"][0]
                                        
    # Execute Test
    t = datetime.now()
    time_str = t.strftime("%Y-%m-%d %H:%M:%S")
    # re-creates the input file text for the report file ... 

    input_str = ''
    for var in input_variables:
        input_str = input_str +  '# ' + var + '=[' + arrayToStr(inputs[var]) + ']\n'

    runBash("echo \"#" + report_filename + "\n# " + time_str  \
                + "\n# input type: [" + INPUT_TYPE + "]\n" + input_str \
                + "\n\n# Added automatically by ccbench.py for the graphing code.\n" \
                + "# The '@' sign lets the parser zero in on special notes to the grapher" \
                + ":\n@NumDataPointsPerSet=[" + str(num_dp_per_set) +  "]\n" 
                + "\" > " + report_filename)
                 
    for app_args in app_args_list:
        architecture.runBenchmark(PROCESSOR, app_bin, app_args, report_filename)



# returns data from the report file in dictionary form (I think)
def readReportFile(report_filename, variables):

    # 3. Extract Data
    file = open(report_filename).readlines() #open the file
        
    # "data" is a 1D dictionary, indexed by Variable.
    data = {}  # Ordered Dict not in Python 2.6 :(


    #initialize arrays
    for variable in variables:
        data[variable] = []
        
    #one program run per line
    #App:[stencil3],NumThreads:[1],AppSizeArg:[32],Time(s):[55.000000]
    for line in file:
        for variable in variables:

            #ignore commented out lines (first char is '#')
            idx = (line.strip()).find("#")
            if idx == 0:
                continue

            #special variables for the graph code
            #i.e., how many data points exist per set
            idx = (line.strip()).find("@")
            if idx == 0:
                if variable in line:
                    data[variable].append(parseValueFromLine(line, variable))

            # (parse for "variable=[1337]")
            # initialize an element in the data matrix to be an array
            if variable in line:
                data[variable].append(parseValueFromLine(line, variable))


    return data
 
# returns inputs from the report file in dictionary form (I think)
def parseReportFileForInputs(report_filename, input_variables):

    # 3. Extract Data
    file = open(report_filename).readlines() #open the file
        
    # "data" is a 1D dictionary, indexed by Variable.
    inputs = {}  # Ordered Dict not in Python 2.6 :(

    #one program run per line
    for line in file:
        for variable in input_variables:

            #ONLY look for variable in "commented" out lines
            idx = (line.strip()).find("#")
            if idx == 0:
                # (parse for "# variable=[1337]")
                # initialize an element in the data matrix to be an array
                if variable in line:
                    inputs[variable] = parseIntArrayFromLine(line, variable)
                    continue


    return inputs

def getReportFileName(app_str, report_dir_path):
    if (REPORT_FILENAME == "none"):
        if (NORUN):
            # if norun, plot using the lastest report_filename 
                report_filename = \
                    getMostRecentReportFile("/" + app_str + "/" + report_dir_path)
        else:   
            report_filename = generateReportFileName(app_str)
    else:   
        # report_filename = os.path.basename(REPORT_FILENAME)
        report_filename = REPORT_FILENAME
    return report_filename


def generateReportFileName(app_str):
    t = datetime.now()
    #time_str = t.strftime("%Y-%m-%d %H:%M:%S")
    time_str = t.strftime("%Y-%m-%d_%Hh%Mm%Ss")
    return "reportfile_" + app_str + "_" + PROCESSOR + "_" + time_str + ".txt"

def generatePlotFileName(app_str):
    t = datetime.now()
    time_str = t.strftime("%Y-%m-%d_%Hh%Mm%Ss")
    return "plot_" + app_str + "_" + PROCESSOR + "_" + time_str

def getMostRecentReportFile(report_dir_path):
    files = runBash("ls -t ../" + report_dir_path)
    files + "\n"
    #print files
    idx = files.find("\n")
    #print idx
    #pull out first file
    #print files[0:idx]
    return files[0:idx]



                
#This idiom means the below code only runs when executed from the command line
if __name__ == '__main__':
  print '--Error: ccbench.py is an include file--'

