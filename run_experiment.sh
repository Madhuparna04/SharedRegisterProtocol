num_server=$1
num_client=$2
rw_per=$3

for (( i=1 ; i<=$num_server ; i++ )); 
do
    ./cmake/build/server $((i-1)) config.json &
done

sleep 5

for (( i=1 ; i<=$num_client ; i++ )); 
do
    ./cmake/build/client $((i-1)) config.json 1000 $rw_per &
done

sleep 20

server_pid=`pgrep server`

for pid in $server_pid
do
    sudo kill -INT $pid
    break
done

sleep 30

server_pid=`pgrep server`

for pid in $server_pid
do
    sudo kill -INT $pid
done

client_pid=`pgrep client`

for pid in $client_pid
do
    sudo kill $pid
done
