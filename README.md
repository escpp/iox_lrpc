# 基于iceoryx的rpc通信系统

# 服务端
```c++
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

```

# 客户端
```c++
#include "iox_lrpc/client.h"
#include "test_lrpc_message.h"

#include <chrono>
#include <iostream>
#include <thread>

void run_client(uint64_t times = 100, const char *app_name="test_lrpc_client")
{
    std::cout << "Starting client..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for server to start

    iox_lrpc::client<AddRequest, AddResponse> client(__FILE__, __LINE__, app_name);

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

int main(int argc, char **argv)
{
    const char *app_name = argc > 1 ? argv[1] : "test_lrpc_client";
    run_client(-1, app_name);
    return 0;
}
```
