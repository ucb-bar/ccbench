##############################################################################
* Christopher Celio
* celio@eecs.berkeley.edu
* UC Berkeley, Parlab
* 2010 Nov
##############################################################################


This repository contains a collection of small micro-kernels to stress-test
memory systems and processors. See **./docs** for more in-depth material on
the theory behind these micro-benchmarks.

The [github wiki](https://github.com/ucb-bar/ccbench/wiki/_pages) shows some of
the latest results on some interesting machines (examples include the RISC-V
Rocket processor and the ARM Cortex A9).

------------------------------------------------------------------------------
## Quick Way to Run Tests
------------------------------------------------------------------------------

See what happens! (works best if matplotlib is installed)

    $ ./runall.sh

The "runall.sh" script is the preferred way to compile benchmarks, run the
tests, collect the results, and plot the results. 
 
You will need matplotlib installed to graph results. The 'open' command is also
used, which only works on OSX (comment it out otherwise). 
 
Set the desired tests to use through the $TEST variable in the script. If
$RUN_LOCAL is set to 1, the tests will run on the local machine.  Otherwise,
you can provide the IP addresses in $remote_hosts to run the tests remotely
(set up loginless login for best results).  You will also need to get the
corresponding $architecture(s) variable to specify the target ISA.


------------------------------------------------------------------------------
## More Controlled Way to Run Tests
------------------------------------------------------------------------------

How to Run the "caches" benchmark (helps deduce access time and cache size):

    $ cd $CCBENCH/caches
    $ make clean; make
    $ ./run_test.py -i large
    $ open plots/caches.pdf

This runs the "caches" benchmark using the "large" input set (see input.txt for
other set types). Data is written to the ./report/report.txt file (actually, a
generated report file based on the current timestamp is used). If matplotlib is
installed, ./run_test.py will also plot the results to a *.pdf file.


If the data is collected on a machine without matplotlib, you can plot the data
by copying the report.txt file to a machine with matplotlib, and then running
./run_test.py using the "-n" flag ("no-run"). 

    laptop$ cd $CCBENCH/caches
    laptop$ scp $LINUX_BOX:~/ccbench/caches/report/report.txt ./report/report.txt
    labtop$ ./run_test.py -n
    laptop$ open plots/caches.pdf

You can feed it a specific report file using "-r my_report_name.txt". 

Use the "-h" flag to ./run_test.py to learn about some of its other options,
which includes specialized annotations for some processors. 
                                                             
------------------------------------------------------------------------------
## Building and Executing ccbench on non-x86 ISAs
------------------------------------------------------------------------------

The ccbench suite currently supports benchmarking processors (or emulators)
running the following ISAs: x86, ARM, Tilera's TILE64, and UCB's RISC-V. 

The easiest method is to use the "runall.sh" top-level script and specify the
machine under the $architecture(s) variable (x86, arm, tile64,riscv). This
drives the Makefile to compile the benchmark as appropriate and supplies the
run_test.py script with the appropriate "-a" flag to invoke the proper target
machine. 

To add your own, new architecture (or to specialize your compiler/invocation
settings), make of a copy of one of the directories in $CCBENCH/arch and
change as needed. For example, you may want to use a different compiler with
its own set of compiler flags, even when running on a particular x86 machine.
Second, modify $CCBENCH/common/Makefile.tests to invoke your new architecture's
Makefile fragment. No modifications are required to the python run_test.py
scripts.

By default, a "generic" architecture setting is used in which gcc is called to
compile the benchmark.  The benchmark is then executed directly on the machine.
This will work for both x86 and ARM. However, to get more specific compiler
flags (as well as for targets that require cross-compiling), it is reccommended
that you specify exactly the desired target architecture.
  
------------------------------------------------------------------------------
## Running Tests Remotely
------------------------------------------------------------------------------

An expected use-case is wanting to easily benchmark a zoo of processors that
reside remotely. Typically, these processors will not have matplotlib
installed. Using "runall.sh", one can easily specify a list of IP addresses,
processor names, and architecture types of each of these target machines. 

You will need to copy the ccbench directory on each remote host machine and
point to this directory using the $HOST_CC_DIR variable in "runall.sh".  Python
is required, but matplotlib is not.

The "runall.sh" script can be run on your personal machine, which then ssh's
into the remote host machine, compiles the benchmark, and invokes the benchmark
on the target processor (either the same as the remote host machine, or could
be an attached accelerator, etc.).  The resulting data is dumped to a report
file, which is then copied back to your local machine. Then, the data can be
graphed locally.

To make life easier, it is recommended that you have passwordless login.

------------------------------------------------------------------------------
## Important Files
------------------------------------------------------------------------------

  - run_tests.py        - main CLI program to run and plot tests. Python 
                            code that invokes C code through bash. Found inside
                            each micro-benchmark directory, and customized to
                            plot data for the given micro-benchmark. 
  - common/             - Contains code shared by all tests.
                            Includes a pthreads barrier implementation for
                            platforms that do not natively support
                            pthreads_barrier_t (i.e., OS X), and an abstracted
                            clock interface.
  - common/Makefile.tests - Main makefile used by all ukernels.
  - reports/report.txt  - Auto-generated by run_tests.py. Stores the
                            results of all of the runs from its latest
                            invocation (actual name is auto-generated and 
                            involves the current timestamp).
  - plots/              - Contains plots generated by run_tests.py
  - input.txt           - Contains input sets used by run_tests.py

 
------------------------------------------------------------------------------
## Current Micro-benchmarks And What They Measure
------------------------------------------------------------------------------

  - caches       - cache sizes, access latencies (pointer chase)
  - cache2cache  - cache-to-cache latency, bandwidth (ping pong arrays 
                    between threads)
  - band_req     - number of outstanding requests (pointer chase with 
                    multiple streams).
  - band_req_mc  - machine total bandwidth (pointer chase with 
                    multiple streams, with multiple threads)
  - strided      - strided memory acccesses: cache sizes, access latencies. 
                    Mostly ineffectual due to prefetching.
  - peakflops    - Prints out max flops of the machine. Does NOT plot 
                    anything (and so doesn't fit within the normal flow 
                    of runall.sh, etc.). Saves results to results.txt file.
  - incluexclu   - figure out if a LLC is inclusive (one thread runs out of 
                    L1, the other thread attempts to blow out the LLC). 
                    (Very experimental, not recommended for use).
  - mem_interleaving- preliminary attempt to measure best interleaving of 
                    multiple threads with different memory distances between 
                    threads (very experimental, not recommended for use).

  
------------------------------------------------------------------------------
## Running benchmarks on the RISC-V emulator
------------------------------------------------------------------------------

By setting the "architecture" variable as RISC-V (e.g., runall.sh), ccbench
will run on the RISC-V processor "emulator" binary.*

Requirements:
   - Your RISC-V C++ emulator binary must be named "emulator".
   - The "emulator" binary must be in your path.
   - The "dramsim2_ini" directory should be located in the same directory as
      the benchmark binary (dramsim2 is used to simulate the off-chip memory).
      This means a copy must exist in $CCBENCH/caches, $$CCBENCH/band_req, etc.
      Hopefully this restriction will be addressed at a later date.
   - patience. You CANNOT run RISC-V emulator using the same input sets you use
      with the other processors. We're talking a ~10,000x difference in run-time.


*Unless you specify the "proc" is "spike", in which case the RISC-V ISA
simulator spike will be used.  It is recommended that you begin all tests with
"spike" first, as it is ~1000x faster than "emulator" and can suss out any bugs
or issues that may arise. 


------------------------------------------------------------------------------
## A note about RISC-V support
------------------------------------------------------------------------------
 
Currently, this suite provides no support for multi-thread RISC-V operations.
For example, the barrier code is completely ifdef'ed out. Currently, ccbench
runs on top of the RISC-V proxy kernel (pk).


------------------------------------------------------------------------------
## Additional Information 
------------------------------------------------------------------------------

Buyer beware. Feel free to provide feedback as well as contribute to.

The CS267 report provided is out-of-date, so do not completely trust the
graphs/results provided in it.


------------------------------------------------------------------------------
## TODO
------------------------------------------------------------------------------

  - make easier to use, extend
  - improve RISC-V code performance (currently too many instructions in inner
        loops, etc.).
  - improve RISC-V support (multi-thread, dramsim2 installation, etc.)
  - modify peakflops to excercise FMA unit
  - modify peakflops to better fit with other benchmarks, more capable of
        compiling to all architectures
  - serial flag for c2c bandwidth (see if matchs intel's)
  - make c2c pick between lat and band (or run and report both numbers?)
  - different strides for c2c
  - more work on mem_interleaving
  - measure cache2cache with MSR prefetching turned off

