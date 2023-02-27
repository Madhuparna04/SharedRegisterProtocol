num_client=$1
rw_per=$2

ops=`echo "scale=0 ; 10240 / $num_client" | bc`

for (( i=1 ; i<=$num_client ; i++ )); 
do
    ./cmake/build/client $((i-1)) remote_config.json $ops $rw_per $num_client &
done

sleep 200

client_pid=`pgrep client`

for pid in $client_pid
do
    sudo kill $pid
done