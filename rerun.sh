#/bin/bash
# "rerun.sh" script runs through all $TESTS on the local machine, re-plotting
# data from existing report files. Excellent for tweaking plots, and then
# regraphing the whole dataset.

# Automagically finds the NUM_REPLOT most recent experiment data files, and replots them.
# Auto-figures out which processor was targetted, etc. (based on naming scheme)
let num_replot=6
#let num_replot=1


#TESTS=(caches band_req band_req_mc cache2cache mem_interleaving)
#TESTS=(band_req_mc)
TESTS=(caches)
#TESTS=(caches band_req)
#TESTS=(cache2cache)
HOST_CC_DIR="~/ccbench/"
CC_DIR="./"


echo "*****************************************************************"
echo "*****************************************************************"
echo "Replotting" $num_replot "Most Recent Experiments"
echo "*****************************************************************"



   
for test in ${TESTS[@]}; do
   time_stamp=$(date +%Y-%m-%d_%Hh%Mm%Ss)
   report_name=reportfile_${proc}_${time_stamp}.txt
   plot_name=plot_${test}_${proc}_${time_stamp}
   echo " "
   echo "*****************************************************************"
   echo "Re-running Test: " $test "  " $time_stamp
   echo " "
   cd $CC_DIR/$test;

   echo "*****************************************************************"
   echo 
   let COUNT=0

   datafiles=`ls -t ./reports/`
   for datafile in $datafiles; do
     
      echo "Processing file ($COUNT):"$datafile
      #  (#) gives trim to end, (%) gives trim from beginning
      # search for proc name by "reportfile_PROCNAME_...."
      proc=${datafile#reportfile_${test}_*}
      proc=${proc%_*_*}
      echo "                   |"${proc}
      time=${datafile#reportfile_${test}_*_}
#      echo "                   |"${time}
      time=${time%*.txt}
#      echo "                   |"${time}
      
      plotname=plot_${test}_${proc}_${time}
      ./run_test.py -p ${proc} -r ${datafile} -o ${plotname} -n; 
      open ./plots/${plotname}.pdf; 
             
      let COUNT=COUNT+1
      if [ $COUNT -eq $num_replot ]; then
         break
      fi

   done

   cd ..;
done


