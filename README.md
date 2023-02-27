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

Run `./run_experiment.sh 3 2 40`
to launch 3 servers, 2 clients and each client performing 40% reads and 60% writes.

## CloudLab Experiment

1. Edit the remote_config.json file to update the ip address of the 5 replicas running the server.

`cd cmake/build`
`./server <SERVER_ID> ../../remote_config.json`

2. After starting the servers, run the followig on a different node for running 2 clients with 50% R/W.:

`./remote_experiment.sh 2 50`

Repeat for 1,2,4,8,16 and 32 clients

Repeat for 100,50,0 (100% Reads, 50% R/W, 0% Reads (100%Writes))


## Plot latency and throughtput Graphs

To plot the graphs run the experiment with 1,2,4,8,16 and 32 clients. The script will split 10240 operations between the number of clients.

`python3 graph.py`

PLot will be saved in latency_throughput.png