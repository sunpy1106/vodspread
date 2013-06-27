#!/bin/bash
cd /home/sunpy/vodspread 
clientConfig=/home/sunpy/vodspread/config/simulator.ini
serverConfig=/home/sunpy/vodspread/config/simulator.ini
client_number=150
resource_number=50
zipfRange=(729) 
for zipf in ${zipfRange[@]}; do
    echo $zipf
done

algorithm_set=(  DR  ) 

for algorithm in ${algorithm_set[@]};do
    echo $algorithm
done

poisson_set=(1000  )

for lambda in ${poisson_set[@]}; do
    echo $lambda
done

#for DW LFRU DR
period_set=(  20 )
#for LRFU
lambda_set=( 700 1000)
#for DR
beta_set=(10)
#for DW LFRU LRFU
thresh_set=(800 850 900 950)
sed -i 's/\(clientNumber = \).*/\1 '$client_number'/g' $clientConfig
sed -i 's/\(resourceNumber = \).*/\1 '$resource_number'/g' $clientConfig
sed -i 's/\(resourceNumber = \).*/\1 '$resource_number'/g' $serverConfig

resultHome=/home/sunpy/vodspread/data
if [ ! -e $resultHome ]; then
    echo "create directory $resultHome "
    mkdir $resultHome
else
    echo "$resultHome exist"
fi
#for each client ,the zipf determine which file it will play 
for zipf in ${zipfRange[@]}; do
    sed -i 's/\(zipfParameter = \).*/\1 '$zipf'/g' $clientConfig
#   mkdir $sub_circle_dir;
# for eache client ,zeta ,sigma ,playtoplay ,playTobackward,playtoforward will determine it's play action.
    for plambda in ${poisson_set[@]}; do
        for algorithm in ${algorithm_set[@]}; do
            sed -i 's/\(spreadAlgorithm = \).*/\1 '$algorithm'/g' $serverConfig
			if [ $algorithm = "RE" ] ; then 
				./server &
				sleep 5s
                ./client &
                sleep 305s
                killall client
                killall server
                sleep 10s
			elif [ $algorithm = "LRFU" ]; then
				for lambda in ${lambda_set[@]}; do
					sed -i 's#\(lambda = \).*#\1 '"$lambda"'#g' $serverConfig
					for thresh in ${thresh_set[@]}; do
						sed -i 's#\(loadThresh = \).*#\1 '"$thresh"'#g' $serverConfig
						./server &
						sleep 5s
						./client &
						sleep 305s
						killall client
						killall server
						sleep 10s
					done
				done
			elif  [ $algorithm = "DR" ]; then 

				for period in ${period_set[@]};do
					sed -i 's/\(period = \).*/\1 '$period'/g' $serverConfig
					for beta in ${beta_set[@]};do
						sed -i 's/\(DRBeta = \).*/\1 '$beta'/g' $serverConfig
                        ./server &
						sleep 5s
                        ./client &
                        sleep 305s
                        killall client
                        killall server
                        sleep 10s
					done
				done

            elif [ $algorithm = "DW" ] || [ $algorithm = "LFRU" ]; then     
				for period in ${period_set[@]};do
					sed -i 's/\(period = \).*/\1 '$period'/g' $serverConfig
					for thresh in ${thresh_set[@]}; do
						sed -i 's#\(loadThresh = \).*#\1 '"$thresh"'#g' $serverConfig
	                    ./server &
						sleep 5s
	                    ./client &
	                    sleep 305s
	                    killall client
	                    killall server
	                    sleep 10s
					done
				done
			fi
		done
	done
done

