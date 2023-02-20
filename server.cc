#include <string>

#include <grpcpp/grpcpp.h>
#include "abd.grpc.pb.h"
#include <map>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using abd::KeyValueStore;
using abd::GetPhaseRequest;
using abd::GetPhaseResponse;
using abd::SetPhaseRequest;
using abd::SetPhaseResponse;

using namespace std;

unordered_map<int,int> keyValueStore;
unordered_map<int,int> lastUpdatedStore;

class KeyValueStoreImplementation final : public abd::KeyValueStore::Service {

    Status get_phase(ServerContext* context, const GetPhaseRequest* request, GetPhaseResponse* response) {
        int client = request->client();
        int server = request->server();
        int request_id = request->request_id();
        int key = request->key();

        int val = 0;
        int lastUpdated = 0;
        if(keyValueStore.find(key) != keyValueStore.end()) {
            val = keyValueStore[key];
            response->set_is_key_present(true);
        } else {
            // Key is not present already - so get phase will return nothing
            response->set_is_key_present(false);
            return Status::OK;
        }

        if(lastUpdatedStore.find(key) != lastUpdatedStore.end()) {
            lastUpdated = lastUpdatedStore[key];
        }

        response->set_client(client);
        response->set_server(server);
        response->set_request_id(request_id);
        response->set_key(key);
        std::cout<<"Sending val for key "<<key<<" = "<<val<<"\n";
        response->set_value(val);
        response->set_local_timestamp(lastUpdated);
        return Status::OK;
    }

    Status set_phase(ServerContext* context, const SetPhaseRequest* request, SetPhaseResponse* response) {
        int client = request->client();
        int server = request->server();
        int request_id = request->request_id();
        int key = request->key();
        int value = request->value();
        int client_timestamp = request->local_timestamp();
        int final_value = -1;

        int lastUpdated = -1;
        if(lastUpdatedStore.find(lastUpdated) != lastUpdatedStore.end()) {
            lastUpdated = lastUpdatedStore[key];
            final_value = keyValueStore[key];
        }

        // TODO: Equals condition?
        if(lastUpdated < client_timestamp) {
            keyValueStore[key] = value;
            final_value = value;
            lastUpdatedStore[key] = client_timestamp;
        }

        std::cout<<"Sending val for key "<<key<<" = "<<final_value<<"\n";
        response->set_client(client);
        response->set_server(server);
        response->set_request_id(request_id);
        response->set_key(key);
        response->set_value(final_value);
        response->set_local_timestamp(max(client_timestamp, lastUpdatedStore[key]));
        return Status::OK;
    }
};

void Run(std::string address) {
    KeyValueStoreImplementation service;

    ServerBuilder builder;

    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on port: " << address << std::endl;

    server->Wait();
}

int main(int argc, char** argv) {
    Run(argv[1]);

    return 0;
}

