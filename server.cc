#include <string>

#include <grpcpp/grpcpp.h>
#include "abd.grpc.pb.h"
#include <map>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using abd::KeyValueStore;
using abd::SetRequest;
using abd::SetResponse;
using abd::GetRequest;
using abd::GetResponse;

using namespace std;

unordered_map<int,int> keyValueStore;

class KeyValueStoreImplementation final : public abd::KeyValueStore::Service {
    Status set( ServerContext* context, const SetRequest* request, SetResponse* response){
        keyValueStore[request->key()] = request->value();
        return Status::OK;
    } 
    Status get( ServerContext* context, const GetRequest* request, GetResponse* response){
        response->set_value(keyValueStore[request->key()]);
        return Status::OK;
    } 
};

void Run() {
    std::string address("0.0.0.0:5000");
    KeyValueStoreImplementation service;

    ServerBuilder builder;

    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on port: " << address << std::endl;

    server->Wait();
}

int main(int argc, char** argv) {
    Run();

    return 0;
}

