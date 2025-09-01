#include "iox_lrpc/server.h"
#include "test_lrpc_message.h"

#include <chrono>
#include <iostream>
#include <thread>

void run_server(uint64_t times = 100)
{
    std::cout << "Starting server..." << std::endl;

    // 模板参数唯一确定服务器实例系统全局唯一
    iox_lrpc::server<AddRequest, AddResponse> server;
    iox_lrpc::server<AddRequest, AddResponse> server2;

    for (uint64_t i = 0; i < times; ++i)
    {
        bool success = server.recv(
            [](const AddRequest& req, AddResponse& resp) {
                std::cout << "Server received request: " << req.a << " + " << req.b << std::endl;
                resp.result = req.a + req.b;
                std::cout << "Server sending response: " << resp.result << std::endl;
                return true;
            },
            111);     
            
            bool success2 = server2.recv(
            [](const AddRequest& req, AddResponse& resp) {
                std::cout << "Server2 received request: " << req.a << " + " << req.b << std::endl;
                resp.result = req.a + req.b;
                std::cout << "Server2 sending response: " << resp.result << std::endl;
                return true;
            },
            111);

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