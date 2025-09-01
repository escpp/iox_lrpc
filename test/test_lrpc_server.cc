#include "iox_lrpc/server.h"
#include "test_lrpc_message.h"

#include <chrono>
#include <iostream>
#include <thread>

void run_server(uint64_t times = 100)
{
    std::cout << "Starting server..." << std::endl;

    iox_lrpc::server<AddRequest, AddResponse> server;

    for (uint64_t i = 0; i < times; ++i)
    {
        bool success = server.recv(
            [](const AddRequest& req, AddResponse& resp) {
                std::cout << "Server received request: " << req.a << " + " << req.b << std::endl;
                resp.result = req.a + req.b;
                std::cout << "Server sending response: " << resp.result << std::endl;
                return true;
            },
            5000);

        if (!success)
        {
            std::cout << "Server timeout or error, restarting..." << std::endl;
        }
    }
}

int main(void)
{
    run_server(-1);
    return 0;
}