#include <string>
#include <grpcpp/grpcpp.h>
#include <fstream>
#include "abd.grpc.pb.h"
#include <map>
#include "json.hpp"
using json = nlohmann::json;

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

unordered_map<string,string> keyValueStore;
unordered_map<string,int> lastUpdatedStore;
std::hash<std::string> hasher;
int SERVER_ID;

class KeyValueStoreImplementation final : public abd::KeyValueStore::Service {

    Status GetPhase(ServerContext* context, const GetPhaseRequest* request, GetPhaseResponse* response) {
        int client = request->client();
        int server = request->server();
        int request_id = request->request_id();
        std::string key = request->key();

        // Assuming all keys are present
        std::string val = keyValueStore[key];
        int lastUpdated = lastUpdatedStore[key];

        response->set_client(client);
        response->set_server(server);
        response->set_request_id(request_id);
        response->set_key(key);
        response->set_value(val);
        response->set_local_timestamp(lastUpdated);
        cout<<"Server get phase .." << SERVER_ID<<endl;
        return Status::OK;
    }

    Status SetPhase(ServerContext* context, const SetPhaseRequest* request, SetPhaseResponse* response) {
        int client = request->client();
        int server = request->server();
        int request_id = request->request_id();
        std::string key = request->key();
        std::string value = request->value();
        int client_timestamp = request->local_timestamp();


        int lastUpdated = lastUpdatedStore[key];
        string final_value = keyValueStore[key];
        
        // TODO: Equals condition?
        if(lastUpdated < client_timestamp) {
            keyValueStore[key] = value;
            final_value = value;
            lastUpdatedStore[key] = client_timestamp;
        }

        response->set_client(client);
        response->set_server(server);
        response->set_request_id(request_id);
        response->set_key(key);
        response->set_value(final_value);
        response->set_local_timestamp(max(client_timestamp, lastUpdatedStore[key]));
        cout<<"Server set phase .." << SERVER_ID<<endl;
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

string covert_to_string28(int x) {
    string int_string = std::to_string(x);
    string zeros = "";
    for(int i = 0; i < (28 - int_string.size()); i++) {
        zeros += '0';
    }
    int_string = zeros + int_string;
    return int_string;
}

void init_map() {
    // Initialize the maps
    for(int i = 0; i < 1e6; i++) {
        string key = covert_to_string28(i);
        auto hashed = hasher(key);
        string val = to_string(hashed).substr(0,10);
        keyValueStore[key] = val;
        lastUpdatedStore[key] = -1;
    }
}

int main(int argc, char** argv) {
    
    if (argc != 3) {
        std::cerr << "Usage: ./server"  << " <SERVER_ID>" << " <Config_File>"<< std::endl;
        return 1;
    }
    SERVER_ID = stoi(argv[1]);
    string config_file = argv[2];

    ifstream ifs;
    ifs.open (config_file, ifstream::in);

    json data = json::parse(ifs);
    int num_servers = stoi(data.value("num_servers", "0"));
    if(num_servers <= SERVER_ID)
    {
        std::cerr << "Invalid server Id! Number of servers = " << num_servers << endl;
        return 0;
    }

    string ip_address = data["server_ip_list"][SERVER_ID];
    string port = data["server_port_list"][SERVER_ID];
    string address = ip_address + ":" + port;

    init_map();
    Run(address);

    return 0;
}

