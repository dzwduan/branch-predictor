make clean
make

total_static_mpi=0
total_bimodal_mpi=0
total_gshare_mpi=0
total_local_mpi=0
total_tourn_mpi=0
# total_tage_mpi=0
total_perceptron_mpi=0
declare -a benchmarks
benchmarks=$(ls ../traces)
for benchmark in $benchmarks
do
  echo "Running $benchmark benchmark.."

  output=`bunzip2 -kc ../traces/$benchmark | ./predictor --static`
  echo -n "static:     ";echo $output
  mpi=`echo $output | awk '{print $7;}'`
  total_static_mpi=`echo "$total_static_mpi + $mpi" | bc`

  
  output=`bunzip2 -kc ../traces/$benchmark | ./predictor --bimodal:13`
  echo -n "bimodal:    ";echo $output
  mpi=`echo $output | awk '{print $7;}'`
  total_bimodal_mpi=`echo "$total_bimodal_mpi + $mpi" | bc`

  output=`bunzip2 -kc ../traces/$benchmark | ./predictor --gshare:13`
  echo -n "gshare:     ";echo $output
  mpi=`echo $output | awk '{print $7;}'`
  total_gshare_mpi=`echo "$total_gshare_mpi + $mpi" | bc`

  output=`bunzip2 -kc ../traces/$benchmark | ./predictor --local:10:10`
  echo -n "local:      ";echo $output
  mpi=`echo $output | awk '{print $7;}'`
  total_local_mpi=`echo "$total_local_mpi + $mpi" | bc`
  
  output=`bunzip2 -kc ../traces/$benchmark | ./predictor --tournament:13:10:10`
  echo -n "tournament: ";echo $output
  mpi=`echo $output | awk '{print $7;}'`
  total_tourn_mpi=`echo "$total_tourn_mpi + $mpi" | bc`
  
#   output=`bunzip2 -kc ../traces/$benchmark | ./predictor --tage:12:15:2:7`
#   echo -n "tage:      ";echo $output
#   mpi=`echo $output | awk '{print $7;}'`
#   total_tage_mpi=`echo "$total_tage_mpi + $mpi" | bc`
  
  output=`bunzip2 -kc ../traces/$benchmark | ./predictor --perceptron:8`
  echo -n "perceptron: ";echo $output
  mpi=`echo $output | awk '{print $7;}'`
  total_perceptron_mpi=`echo "$total_perceptron_mpi + $mpi" | bc`
done
echo "=============================="
echo -n "Average misprediction for static predictor is: "
echo "scale=4; $total_static_mpi/6" | bc

echo -n "Average misprediction for bimodal predictor is: "
echo "scale=4; $total_bimodal_mpi/6" | bc

echo -n "Average misprediction for gshare predictor is: "
echo "scale=4; $total_gshare_mpi/6" | bc

echo -n "Average misprediction for local predictor is: "
echo "scale=4; $total_local_mpi/6" | bc

echo -n "Average misprediction for tournament predictor is: "
echo "scale=4; $total_tourn_mpi/6" | bc

# echo -n "Average misprediction for tage is: "
# echo "scale=4; $total_tage_mpi/6" | bc

echo -n "Average misprediction for perceptron is: "
echo "scale=4; $total_perceptron_mpi/6" | bc