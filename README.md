# SharedRegisterProtocol

Shared Register Protocol using gRPC for communication between clients and servers.

## Instructions to run the code

1. Install gRPC : https://grpc.io/docs/languages/cpp/quickstart/
2. Run `./build.sh`
3. `cd cmake/build`
4. Run `./server <SERVER_ID> <Config_file>` (Example: `./server 2 ../../config.json`
 (Server id should be between 0 and num_servers - 1, num_servers declared in config.json))
5. In a new terminal run `./client <CLIENT_ID> <Config_file> <NUM_OPS> <PERCENTAGE OF R/W> <NUM_CLIENTS>` (Example: ./client 4 ../../config.json 100 40 5)


## Experiment

Run `./run_experiment.sh 3 2 40` to launch 3 servers, 2 clients and each client performing 40% reads and 60% writes.

## Plot latency and throughtput Graphs

To plot the graphs run the experiment with 1,2,4,8,16 and 32 clients. The script will split 10240 operations between the number of clients.

`./run_experiment 5 1 50`

`./run_experiment 5 2 50`

`./run_experiment 5 4 50`

`./run_experiment 5 8 50`

`./run_experiment 5 16 50`

`./run_experiment 5 32 50`

Then run the python script:

`python3 graph.py`

Two plots will be saved in latency.png and throughput.png