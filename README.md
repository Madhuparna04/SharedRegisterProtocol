# SharedRegisterProtocol

Shared Register Protocol using gRPC for communication between clients and servers.

## Instructions to run the code

1. Install gRPC : https://grpc.io/docs/languages/cpp/quickstart/
2. Run `./build.sh`
3. `cd cmake/build`
4. Run `./server <SERVER_ID> <Config_file>` (Example: `./server 2 ../../config.json`
 (Server id should be between 0 and num_servers - 1, num_servers declared in config.json))
5. In a new terminal run `./client <CLIENT_ID> <Config_file> <NUM_OPS> <PERCENTAGE OF R/W>` (Example: ./client 10 ../../config.json 100 40)


## Experiment

Run `./run_experiment.sh 3 2 40` to launch 3 servers, 2 clients and each client performing 40% reads and 60% writes.