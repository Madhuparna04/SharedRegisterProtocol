#include <string>

#include <grpcpp/grpcpp.h>
#include "abd.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using abd::KeyValueStore;
using abd::SetRequest;
using abd::SetResponse;
using abd::GetRequest;
using abd::GetResponse;

class KeyValueStoreClient {
    public:
        KeyValueStoreClient(std::shared_ptr<Channel> channel) : stub_(KeyValueStore::NewStub(channel)) {}

    int set(int key, int value) {
        SetRequest request;
        SetResponse response;
        ClientContext context;

        request.set_key(key);
        request.set_value(value);

        Status status = stub_->set(&context, request, &response);

        if(status.ok()){
            return 0;
        } else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return -1;
        }
    }

    int get(int key) {
        GetRequest request;
        GetResponse response;
        ClientContext context;
        request.set_key(key);

        Status status = stub_->get(&context, request, &response);

        if(status.ok()){
            return response.value();
        } else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            return -1;
        }
    }

    private:
        std::unique_ptr<KeyValueStore::Stub> stub_;
};

void Run() {
    std::string address("0.0.0.0:5000");
    KeyValueStoreClient client(
        grpc::CreateChannel(
            address, 
            grpc::InsecureChannelCredentials()
        )
    );

    int response;

    int a = 5;
    int b = 10;

    client.set(a, b);
    response =  client.get(a);
    std::cout << "Key: " << a << " Value: " << " = " << response << std::endl;
}

int main(int argc, char* argv[]){
    Run();

    return 0;
}