num_server=$1
num_writer=$2
num_reader=$3

for (( i=1 ; i<=$num_server ; i++ )); 
do
    ./cmake/build/server $((i-1)) config.json &
done

sleep 2

for (( i=1 ; i<=$num_writer ; i++ )); 
do
    ./cmake/build/client $((i-1)) config.json 5000 0 &
done

sleep 5

total=$((num_writer + num_reader -1))
for i in $(eval echo "{$num_writer..$total}")
do
    ./cmake/build/client $i config.json 0 5000 &
done

sleep 20

server_pid=`pgrep server`

for pid in $server_pid
do
    sudo kill $pid
    break
done

sleep 500

server_pid=`pgrep server`

for pid in $server_pid
do
    sudo kill $pid
done

client_pid=`pgrep client`

for pid in $client_pid
do
    sudo kill $pid
done
