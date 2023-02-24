#include <string>
#include <map>
#include <vector>
#include <thread>
#include <fstream>
#include <random>
#include <mutex>
#include <grpcpp/grpcpp.h>
#include <condition_variable>
#include "abd.grpc.pb.h"
#include "json.hpp"
using json = nlohmann::json;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientAsyncResponseReader;
using grpc::CompletionQueue;

using abd::KeyValueStore;
using abd::GetPhaseRequest;
using abd::GetPhaseResponse;
using abd::SetPhaseRequest;
using abd::SetPhaseResponse;

using namespace std;

std::vector<std::string> servers;
int kExpectedResponses;

// Define a global counter for the number of responses received
std::vector<GetPhaseResponse> get_responses;
std::vector<SetPhaseResponse> set_responses;

// Define a mutex
std::mutex gresponse_mutex;
std::mutex sresponse_mutex;
std::condition_variable gresponse_count_cv;
std::condition_variable sresponse_count_cv;

// TODO: Take from ARGS/develop a class
int MY_CLIENT_ID;
std::unordered_map<int,int> keyTimestamps;
int MY_REQUEST_ID = 0;
int NUM_WRITES = 0;
int NUM_READS = 0;

class KeyValueStoreClient {
    public:
        KeyValueStoreClient(std::shared_ptr<Channel> channel) : stub_(KeyValueStore::NewStub(channel)) {}

    void GetPhase(int client, int server, int requestId, int key, std::function<void(const GetPhaseResponse&)> callback) {

        GetPhaseRequest request;
        request.set_client(client);
        request.set_server(server);
        request.set_request_id(requestId);
        request.set_key(key);

        CompletionQueue cq;
        ClientContext context;

        // Make the asynchronous call
        auto response_reader = stub_->AsyncGetPhase(&context, request, &cq);

        // Provide a tag object to be passed to the callback function
        void* tag = (void*) new CallbackTagGetPhase(callback);

        // Set up the callback function to be called when the response is received
        // Tag has the callback function
        response_reader->Finish(&response_get_, &status_, tag);

        // Wait for the response to be received
        void* got_tag;
        bool ok = false;
        while (cq.Next(&got_tag, &ok)) {
            if (ok) {
                // Call the callback function with the retrieved value
                CallbackTagGetPhase* callback_tag = static_cast<CallbackTagGetPhase*>(got_tag);

                // Response object passed to callback function here
                (callback_tag->callback)(response_get_);
                delete callback_tag;
                break;
            }
        }
    }

    void SetPhase(int client, int server, int requestId, int key, int value, int local_timestamp, std::function<void(const SetPhaseResponse&)> callback) {
        SetPhaseRequest request;
        request.set_client(client);
        request.set_server(server);
        request.set_request_id(requestId);
        request.set_key(key);
        request.set_value(value);
        request.set_local_timestamp(local_timestamp);

        ClientContext context;
        CompletionQueue cq;

        // Make the asynchronous call
        auto response_reader = stub_->AsyncSetPhase(&context, request, &cq);

        // Provide a tag object to be passed to the callback function
        void* tag = (void*) new CallbackTagSetPhase(callback);

        // Set up the callback function to be called when the response is received
        // Tag has the callback function
        response_reader->Finish(&response_set_, &status_, tag);

        // Wait for the response to be received
        void* got_tag;
        bool ok = false;
        while (cq.Next(&got_tag, &ok)) {
            if (ok) {
                // Call the callback function with the retrieved value
                CallbackTagSetPhase* callback_tag = static_cast<CallbackTagSetPhase*>(got_tag);

                // Response object passed to callback function here
                (callback_tag->callback)(response_set_);
                delete callback_tag;
                break;
            }
        }
    }

    private:
        std::unique_ptr<KeyValueStore::Stub> stub_;
        GetPhaseResponse response_get_;
        SetPhaseResponse response_set_;
        Status status_;

        // The tag object to be passed to the callback function
        class CallbackTagGetPhase {
            public:
                CallbackTagGetPhase(std::function<void(const GetPhaseResponse&)> cb) : callback(cb) {}
                std::function<void(const GetPhaseResponse&)> callback;
        };

        // The tag object to be passed to the callback function
        class CallbackTagSetPhase {
            public:
                CallbackTagSetPhase(std::function<void(const SetPhaseResponse&)> cb) : callback(cb) {}
                std::function<void(const SetPhaseResponse&)> callback;
        };
};

void HandleGetPhaseResponse(const GetPhaseResponse& response) {
    if (response.request_id() == MY_REQUEST_ID) {
        // Lock for accesing the get_responses vector bec it is accessed by many threads
        // gresponse_mutex.lock();
        std::cout<<"Recieved get response for "<<MY_REQUEST_ID<<"\n";
        get_responses.push_back(response);
        // gresponse_mutex.unlock();

        if(get_responses.size() == kExpectedResponses) {
            // Signal - main thread will resume execution now
            // gresponse_count_cv.notify_all();
            gresponse_mutex.unlock();
            std::cout<<"Now ending get phase "<<MY_REQUEST_ID<<"\n";
            // signal_get_done.unlock();
        }
    }
}

void HandleSetPhaseResponse(const SetPhaseResponse& response) {
    if (response.request_id() == MY_REQUEST_ID) {
        // Lock for accesing the get_responses vector bec it is accessed by many threads
        // sresponse_mutex.lock();
        std::cout<<"Recieved set response for "<<MY_REQUEST_ID<<"\n";
        set_responses.push_back(response);
        // sresponse_mutex.unlock();

        if(set_responses.size() == kExpectedResponses) {
            // Signal - main thread will resume execution now
            // sresponse_count_cv.notify_all();
            sresponse_mutex.unlock();
            std::cout<<"Now ending set phase "<<MY_REQUEST_ID<<"\n";
            // signal_set_done.unlock();
        }
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

        std::cout<<"Sending <<"<<i<<" get request for request ID "<<requestId<<"\n";
        client.GetPhase(client_id, i, requestId, key, HandleGetPhaseResponse);         
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
        std::cout<<"Sending <<"<<i<<" set request for request ID "<<requestId<<"\n";
        client.SetPhase(client_id, i, requestId, key, value, local_timestamp, HandleSetPhaseResponse);
    }
}

void StartGetThread(int key) {
    // Start of Get Phase
    // This lock will only be released once Get Phase get enough responses
    // std::cout<<"Taking get lock\n";
    // signal_get_done.lock();
    RunGetPhase(servers, MY_CLIENT_ID, MY_REQUEST_ID, key);
}

void StartSetThread(int key, int val) {
    // Start of Set Phase
    // This lock will only be released once Set Phase get enough responses
    // signal_set_done.lock();
    RunSetPhase(servers, MY_CLIENT_ID, MY_REQUEST_ID, key, val, keyTimestamps[key]);
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

    kExpectedResponses = ((servers.size()/2) + 1);
}

void do_read_and_write(char op, int repeat) {
    int key;
    int val;
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(1,100); 
    for(int i = 0; i < repeat ; ++i) {
        
        std::cout<<"====Starting new request====\n";
        key = dist(rng);    
        val = dist(rng);

        // Start of Get Phase
        std::thread get_phase_thread(StartGetThread, key); 
        {
            gresponse_mutex.lock();
        };

        std::cout<<"Main is waiting for the Get signal\n";
        gresponse_mutex.lock();

        get_phase_thread.detach();
        std::cout<<"Main Received Get signal - releasing lock\n";
        gresponse_mutex.unlock();

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
        // MY_REQUEST_ID++;

        if(op == 'R') {
            if(isKeyPresentInAny) {
                // Value corresponding to the largest timestamp among majority
                int value_read = get_responses[max_in].value();

                // Start Set Phase in a thread
                std::thread set_phase_thread(StartSetThread, key, value_read);
                {
                    sresponse_mutex.lock();
                };

                std::cout<<"Main is waiting for the Set signal\n";
                sresponse_mutex.lock();
                std::cout<<"Main Received Set signal - releasing lock\n";
                sresponse_mutex.unlock();

                // StartSetThread(key, value_read);

                // while(set_responses.size() < kExpectedResponses);

                // std::unique_lock<std::mutex> slock(sresponse_mutex);
                // gresponse_count_cv.wait(slock, []{return set_responses.size() == kExpectedResponses;});

                // set_phase_thread.join();
                set_phase_thread.detach();

                std::cout<<"Client Id "<<MY_CLIENT_ID<<": R: value for key = "<<key<<" is "<<value_read<<" Op No. "<<i<<"\n";
            } else {
                std::cout<<"Client Id "<<MY_CLIENT_ID<<": R: key = "<<key<<" is not present."<<" Op No. "<< i << "\n";
            }
        } else if(op == 'W'){
            // Start Set Phase in a thread
            std::thread set_phase_thread(StartSetThread, key, val);
            {
                sresponse_mutex.lock();
            };

            std::cout<<"Main is waiting for the Set signal\n";
            sresponse_mutex.lock();
            std::cout<<"Main Received Set signal - releasing lock\n";
            sresponse_mutex.unlock();

            // while(set_responses.size() < kExpectedResponses);
            // std::unique_lock<std::mutex> slock(sresponse_mutex);
            // gresponse_count_cv.wait(slock, []{return set_responses.size() == kExpectedResponses;});

            // set_phase_thread.join();
            std::cout<<"Client Id "<<MY_CLIENT_ID<<": Write complete for key = "<<key<<" Value = "<< val<<" Op No."<<i<<"\n";
            set_phase_thread.detach();
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