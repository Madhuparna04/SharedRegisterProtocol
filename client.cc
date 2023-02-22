#include <string>
#include <map>
#include <vector>
#include <thread>
#include <fstream>
#include <random>
#include <grpcpp/grpcpp.h>
#include <condition_variable>
#include "abd.grpc.pb.h"
#include "json.hpp"
using json = nlohmann::json;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using abd::KeyValueStore;
using abd::GetPhaseRequest;
using abd::GetPhaseResponse;
using abd::SetPhaseRequest;
using abd::SetPhaseResponse;

using namespace std;
// Define the number of requests to send and the number of responses to expect
// TODO: CHANGE
std::vector<std::string> servers;// = {"0.0.0.0:3000", "0.0.0.0:3002", "0.0.0.0:3004"};
const int kExpectedResponses = ((servers.size()/2) + 1);

// Define a global counter for the number of responses received
std::vector<GetPhaseResponse> get_responses;
std::vector<SetPhaseResponse> set_responses;

// Define a mutex and condition variable to synchronize access to response_count
std::mutex gresponse_mutex;
std::mutex sresponse_mutex;
std::condition_variable gresponse_count_cv;
std::condition_variable sresponse_count_cv;

// TODO: Take from ARGS/develop a class
int MY_CLIENT_ID;
std::unordered_map<int,int> keyTimestamps;
int MY_REQUEST_ID;
int NUM_WRITES = 0;
int NUM_READS = 0;

class KeyValueStoreClient {
    public:
        KeyValueStoreClient(std::shared_ptr<Channel> channel) : stub_(KeyValueStore::NewStub(channel)) {}

    GetPhaseResponse get_phase(int client, int server, int requestId, int key) {
        GetPhaseResponse response;
        GetPhaseRequest request;
        ClientContext context;

        request.set_client(client);
        request.set_server(server);
        request.set_request_id(requestId);
        request.set_key(key);

        Status status = stub_->get_phase(&context, request, &response);

        return response;

        // if(status.ok()){
        //     return response;
        // } else {
        //     std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        //     return response;
        // }
    }

    SetPhaseResponse set_phase(int client, int server, int requestId, int key, int value, int local_timestamp) {
        SetPhaseResponse response;
        SetPhaseRequest request;
        ClientContext context;

        request.set_client(client);
        request.set_server(server);
        request.set_request_id(requestId);
        request.set_key(key);
        request.set_value(value);
        request.set_local_timestamp(local_timestamp);

        Status status = stub_->set_phase(&context, request, &response);
        return response;

        // if(status.ok()){
        //     return response;
        // } else {
        //     std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        //     return response;
        // }
    }

    private:
        std::unique_ptr<KeyValueStore::Stub> stub_;
};

void HandleGetPhaseResponse(const GetPhaseResponse& response) {
    // Increment the response count
    std::unique_lock<std::mutex> glock(gresponse_mutex);
    // std::cout<<"New response came. Adding to the vector "<<"\n";
    get_responses.push_back(response);
    // Notify all waiting threads if we have received the expected number of responses
    if (get_responses.size() == kExpectedResponses) {
        // std::cout<<"I am get notifying\n";
        gresponse_count_cv.notify_all();
    }
}

void HandleSetPhaseResponse(const SetPhaseResponse& response) {
    // Increment the response count
    std::unique_lock<std::mutex> slock(sresponse_mutex);
    // std::cout<<"New response came. Adding to the vector "<<"\n";
    set_responses.push_back(response);
    // Notify all waiting threads if we have received the expected number of responses
    if (set_responses.size() == kExpectedResponses) {
        // std::cout<<"I am set notifying\n";
        sresponse_count_cv.notify_all();
    }
}

void RunGetPhase(std::vector<std::string> servers, int client_id, int requestId, int key) {
    for(int i = 0; i < servers.size(); i++) {
        KeyValueStoreClient client(
            grpc::CreateChannel(
                servers[i], 
                grpc::InsecureChannelCredentials()
            )
        );

        // std::cout<<"Sending get phase request "<<i<<"\n";
        GetPhaseResponse response = client.get_phase(client_id, i, requestId, key);
        HandleGetPhaseResponse(response);
    }
}


void RunSetPhase(std::vector<std::string> servers, int client_id, int requestId, int key, int value, int local_timestamp) {
    for(int i = 0; i < servers.size(); i++) {
        KeyValueStoreClient client(
            grpc::CreateChannel(
                servers[i], 
                grpc::InsecureChannelCredentials()
            )
        );

        // std::cout<<"Sending set phase request "<<i<<"\n";
        SetPhaseResponse response = client.set_phase(client_id, i, requestId, key, value, local_timestamp);
        HandleSetPhaseResponse(response);
    }
}


void StartGetThread(int key) {
    std::thread getPhaseThread(RunGetPhase, servers, MY_CLIENT_ID, MY_REQUEST_ID, key);
    std::unique_lock<std::mutex> glock(gresponse_mutex);
    // This keeps releasing the lock until the condition is false - 
    // and keeps holding it once it is fulfilled - so other threads 
    // are unable to modify anything
    gresponse_count_cv.wait(glock, []{return get_responses.size() == kExpectedResponses;});
    getPhaseThread.detach();
    // std::cout<<"Ending Get Phase for Key = \n"<<key;
}

void StartSetThread(int key, int value) {
    // Write back phase
    std::thread setPhaseThread(RunSetPhase, servers, MY_CLIENT_ID, MY_REQUEST_ID, key, value, keyTimestamps[key]);
    std::unique_lock<std::mutex> slock(sresponse_mutex);
    // This keeps releasing the lock until the condition is false - 
    // and keeps holding it once it is fulfilled - so other threads 
    // are unable to modify anything
    sresponse_count_cv.wait(slock, []{return set_responses.size() == kExpectedResponses;});
    setPhaseThread.detach();
    // std::cout<<"Hi I have received 2 set responses\n"<<set_responses[0].value()<<"\n"<<set_responses[1].value();
}

void parse_server_address(string config_file) {
    ifstream ifs;
    ifs.open (config_file, ifstream::in);

    json data = json::parse(ifs);
    int num_servers = stoi(data.value("num_servers", "0"));

    for (int i = 0; i < num_servers ; ++i) {
        string ip_address = data["server_ip_list"][i];
        string port = data["server_port_list"][i];
        string address = ip_address + ":" + port;
        servers.push_back(address);
    }
}

void do_read_and_write(char op, int repeat) {
    int key;
    int val;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1,100); 
    for(int i = 0; i < repeat ; ++i) {

        key = dist(rng);
        val = dist(rng);

        // Start of Get Phase
        StartGetThread(key);

        // Find largest timestamp among received responses
        bool isKeyPresentInAny = false;
        int max_timestamp = -100;
        int max_in = -1;
        for(int i = 0; i < get_responses.size(); i++) {
            isKeyPresentInAny |= get_responses[i].is_key_present();
            if(get_responses[i].local_timestamp() > max_timestamp) {
                max_timestamp = get_responses[i].local_timestamp();
                max_in = i;
            } 
        }

        // Local timestamp changes to max + 1 for the key
        keyTimestamps[key] = max_timestamp + 1;

        if(op == 'R') {
            if(isKeyPresentInAny) {
                // Value corresponding to the largest timestamp among majority
                int value_read = get_responses[max_in].value();

                // Start Set Phase
                StartSetThread(key, value_read);
                std::cout<<"R: value for key = "<<key<<" is "<<value_read<<"\n";
            } else {
                std::cout<<"R: key = "<<key<<" is not present. \n";
            }
        } else if(op == 'W'){
            std::cout<<"Enter value: ";
            std::cin>>val;
            
            // Start Set Phase
            StartSetThread(key, val);
            std::cout<<"Write complete.\n";
        } else {
            std::cout<<"Invalid Operation, terminating client.\n";
            return;
        }
        
        get_responses.clear();
        set_responses.clear();
        MY_REQUEST_ID++;
    }
}

int main(int argc, char* argv[]){

    if (argc != 5) {
        std::cerr << "Usage: ./client"  << " <Client_Id> <Config_file> <NUM_WRITES> <NUM_READS>" << std::endl;
        return 1;
    }
    MY_CLIENT_ID = std::stoi(argv[1]);
    parse_server_address(argv[2]);
    NUM_WRITES = std::stoi(argv[3]);
    NUM_READS = std::stoi(argv[4]);
    MY_REQUEST_ID = 1;

    do_read_and_write('W', NUM_WRITES);
    do_read_and_write('R', NUM_READS);

    return 0;
}