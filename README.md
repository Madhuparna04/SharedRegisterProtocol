# SharedRegisterProtocol

Shared Register Protocol using gRPC for communication between clients and servers.

## Instructions to run the code

1. Install gRPC : https://grpc.io/docs/languages/cpp/quickstart/
2. Run `./run.sh`
3. `cd cmake/build`
4. Run `./server <IP:PORT>` (Example: ./server 0.0.0.0:3000)
5. In a new terminal run `./client <CLIENT_ID>` (Example: ./client 10)
