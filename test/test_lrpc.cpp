#include "lrpc/client.h"
#include "lrpc/server.h"

#include <iostream>
#include <thread>
#include <chrono>

// Simple request and response structures
struct AddRequest {
    int a;
    int b;
};

struct AddResponse {
    int result;
};

void run_server() {
    std::cout << "Starting server..." << std::endl;
    
    en_lrpc::server<AddRequest, AddResponse> server;
    
    while (true) {
        bool success = server.recv([](const AddRequest& req, AddResponse& resp) {
            std::cout << "Server received request: " << req.a << " + " << req.b << std::endl;
            resp.result = req.a + req.b;
            std::cout << "Server sending response: " << resp.result << std::endl;
            return true;
        }, 5000);
        
        if (!success) {
            std::cout << "Server timeout or error, restarting..." << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void run_client() {
    std::cout << "Starting client..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for server to start
    
    en_lrpc::client<AddRequest, AddResponse> client;
    
    for (int i = 0; i < 5; ++i) {
        AddRequest req;
        req.a = i;
        req.b = i * 2;
        
        std::cout << "Client sending request: " << req.a << " + " << req.b << std::endl;
        
        bool success = client.send(req, [](const AddResponse& resp) {
            std::cout << "Client received response: " << resp.result << std::endl;
        }, 3000);
        
        if (!success) {
            std::cout << "Client request failed or timeout" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    std::cout << "LRPC Test Application" << std::endl;
    std::cout << "1. Run server" << std::endl;
    std::cout << "2. Run client" << std::endl;
    std::cout << "Enter choice: ";
    
    int choice;
    std::cin >> choice;
    
    if (choice == 1) {
        run_server();
    } else if (choice == 2) {
        run_client();
    } else {
        std::cout << "Invalid choice" << std::endl;
    }
    
    return 0;
}
