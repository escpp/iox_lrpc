#include "iox_lrpc/client.h"
#include "test_lrpc_message.h"

#include <chrono>
#include <iostream>
#include <thread>

void run_client(uint64_t times = 100)
{
    std::cout << "Starting client..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for server to start

    iox_lrpc::client<AddRequest, AddResponse> client(__FILE__, __LINE__);

    for (uint64_t i = 0; i < times; ++i)
    {
        AddRequest req;
        req.a = i;
        req.b = i * 2;

        std::cout << "Client sending request: " << req.a << " + " << req.b << std::endl;

        bool success = client.send(
            req,
            [](const AddResponse& resp) { std::cout << "Client received response: " << resp.result << std::endl; },
            3000);

        if (!success)
        {
            std::cout << "Client request failed or timeout" << std::endl;
        }
    }
}

int main()
{
    run_client(-1);
    return 0;
}
