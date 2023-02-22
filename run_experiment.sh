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
    ./cmake/build/client $((i-1)) config.json 100 0 &
done

total=$((num_writer + num_reader))
for i in $(eval echo "{$num_writer..$total}")
do
    ./cmake/build/client $((i-1)) config.json 0 10 &
done

sleep(5)