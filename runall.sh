#/bin/bash
# "runall.sh" script runs through all $TESTS, either on local machine if RUN_LOCAL=1, or on
# a list of remote machines otherwise. 

# **** NOTE: ASSUMES RUNNING ON MAC OS X, with matplotlib installed **** 
# - If local, opens the generated plots (only works on OS X using the "open" command).
# - If remote, runs tests remotely, then pulls report.txt file to the local
#     machine, plots data locally, and then opens pdf file on local machine.
#     Assumes remote machine doesn't have matplotlib for plotting, so that 
#     work is done locally.

# comment out the necessary steps in the code below if your machine doesn't
# support the above operations. 



#run tests on local machine, or run tests on remote machines?
RUN_LOCAL=0


# Which tests and input sizes do we want to use?
#TESTS=(caches band_req band_req_mc cache2cache mem_interleaving)
TESTS=(band_req)
INPUTSIZE="riscv"
#INPUTSIZE="large"

# Information for running on the local machine
local_proc="ivybridge"         
architecture="x86"

# Information for running on remote host machines
HOST_CC_DIR="~/ccbench/"
#remote_hosts=($BOXBORO $CUDA1 $EMERALD $BRIDGE $CUDA1 $CUDA1)
#remote_procs=(boxboro cuda1 emerald bridge tilera-l3 tilera)
#remote_archs=(x86 x86 x86 x86 tile64 tile64)

remote_hosts=($A3)
remote_procs=(spike)
remote_archs=(riscv)


if [ $RUN_LOCAL -eq 1 ]
then

   for test in ${TESTS[@]}; do
      time_stamp=$(date +%Y-%m-%d_%Hh%Mm%Ss)
      report_name=reportfile_${test}_${local_proc}_${time_stamp}.txt
      plot_name=plot_${test}_${local_proc}_${time_stamp}
      echo " "
      echo "*****************************************************************"
      echo "RUNNING TEST: " $test "  " $time_stamp  " : " 
      echo "*****************************************************************"
      echo " "
      echo $@
      cd $test; make clean; make ARCH=${architecture} && ./run_test.py -i $INPUTSIZE -p $local_proc -a $architecture -r ${report_name} -o ${plot_name} && open plots/${plot_name}.pdf; cd ..;
      echo " "
   done

else

   for (( i=0; i < ${#remote_hosts[@]}; i++ )); do
      host=${remote_hosts[$i]}
      proc=${remote_procs[$i]}
      architecture=${remote_archs[$i]}
      echo " "
      echo "*****************************************************************"
      echo "USING HOST: " $host " for arch: " ${architecture}
      echo "*****************************************************************"
      echo " "
      for test in ${TESTS[@]}; do
         time_stamp=$(date +%Y-%m-%d_%Hh%Mm%Ss)
         report_name=reportfile_${test}_${proc}_${time_stamp}.txt
         plot_name=plot_${test}_${proc}_${time_stamp}
         echo " "
         echo "*****************************************************************"
         echo "RUNNING TEST: " $test "  " $time_stamp
         echo " "
         ssh $host "export PROC=${proc}; cd $HOST_CC_DIR/$test; make clean; make ARCH=${architecture}; ./run_test.py -i $INPUTSIZE -p $proc -a ${architecture} -r ${report_name} -o ${plot_name}; cd ..;"
         echo "Pulling report.txt file back to local machine..."
         scp $host:$HOST_CC_DIR/$test/reports/${report_name} ./$test/reports/${report_name};
         cd $test; ./run_test.py -a ${architecture} -p $proc -r ${report_name} -o ${plot_name} -n; open ./plots/${plot_name}.pdf; cd ..;
      done
   done

fi

