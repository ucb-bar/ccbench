#! /usr/bin/env python
 
# Author: Christopher Celio
# Date  : 2012 Jan 1
#   
# Store data about different processors in here, to assist in plotting
# overlays. 
 
try:
    import numpy as np
except:
    #this condition is handled elsewhere
    dontworryaboutit=1

#store all processor information in here (as relayed by the specs, not
# empirically discovered)
procinfo = {}

# use a "-1" to signify no L3, etc.
def addProcInfo(proc_label, proc_title_string, l1_sz, l2_sz, l3_sz, l4_sz):
    procinfo[proc_label] = {}
    procinfo[proc_label]["Title"] = proc_title_string
    procinfo[proc_label]["CacheSizes"] = {}
    procinfo[proc_label]["CacheSizes"]["L1"] = l1_sz
    procinfo[proc_label]["CacheSizes"]["L2"] = l2_sz
    procinfo[proc_label]["CacheSizes"]["L3"] = l3_sz
    procinfo[proc_label]["CacheSizes"]["L4"] = l4_sz


addProcInfo("merom",      "Intel Core 2 Duo T7600 (Merom) @ 2.33 GHz",           32, 4096,      -1,      -1)
addProcInfo("ivybridge",  "Intel Core i7 (Ivy Bridge) @ 2.3 GHz",                32,  256,  6*1024,      -1)
addProcInfo("bridge",     "Intel Core i7-2600 (Sandy Bridge) @ 3.40GHz, 4 core", 32,  256,  8*1024,      -1)
addProcInfo("boxboro",    "Intel Xeon E7 4860 (Westmere-EX) @ 2.27 Ghz, 10 core",32,  256, 24*1024, 96*1024)
addProcInfo("emerald",    "Intel Xeon X7560 (Nehalem-EX) @ 2.27GHz, 8 core",     32,  256, 24*1024,      -1)
addProcInfo("cuda1",      "Intel Core 2 Quad @ 2.40Ghz",                         32, 4096,      -1,      -1)
addProcInfo("arrandale",  "Intel Core i7 640M (Arrandale)",                      32,  256,    4096,      -1)
addProcInfo("bloomfield", "Intel Core i7 975 EE (Bloomfield)",                   32,  256,  8*1024,      -1)
#TODO add "l2 slice", and "aggreagte L2s" as labels...
addProcInfo("tilera",     "TilePro64 (Default malloc behavior) @ 700 MHz",        8,   64,    4096,      -1)
addProcInfo("tilera-l3",  "TilePro64 (using 'Hash-for-Home') @ 700 MHz",          8,   64,    4096,      -1)
addProcInfo("tegra2",     "Tegra 2 (ARM Cortex-A9) Dual-core",                   32,  512,      -1,      -1)



#helper functions

def logMidPoint(low, high):
    return np.exp2((np.log2(high) + np.log2(low))/2.0) 


# plot is a handle to the plot to write to
# proc is the processor name
# ylow/yhigh are the y-values to annotate at
#   some text is at the level of yhigh,
#   other text is at the level of ylow

cachelabel = {}
cachelabel["L1"] = "L1 D-Cache"
cachelabel["L2"] = "L2 Cache"
cachelabel["L3"] = "L3 Cache"
cachelabel["L4"] = "Socket"

def cacheSizeString(size):
    if (size >= 1024):
        return "(" + str(size/1024) + " MB)"    
    else:
        return "(" + str(size) + " kB)"    

def plotCacheSizeLines(plt, plot, proc, ylow, yhigh):

    if (not (proc in procinfo)):
        return


    plt.title(procinfo[proc]["Title"], fontstyle='italic')
    cs = "CacheSizes"
    (xlow,xmax) = plt.xlim()
    for cache in ("L1","L2","L3","L4"):
        x = procinfo[proc][cs][cache]
        if (x == -1):
            continue
        plot.axvline(x=x,   ymin=0.05, ymax=.9, linestyle='--', color='black')
        plot.text(logMidPoint(xlow,x), yhigh, cachelabel[cache], horizontalalignment='center',verticalalignment='center')
        plot.text(logMidPoint(xlow,x), ylow, cacheSizeString(x), horizontalalignment='center',verticalalignment='center')
        xlow = x
        xlast = x
    
    plot.text(logMidPoint(xlast,xmax), yhigh,"off-chip", horizontalalignment='center',verticalalignment='center')

