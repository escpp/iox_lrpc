# 基于iceoryx的rpc通信系统

# 服务端
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

    // 传入的__FILE__、__LINE__和app_name确定了client的唯一实例id用于端到端rpc通信
    iox_lrpc::client<AddRequest, AddResponse> client(__FILE__, __LINE__, app_name);

    for (uint64_t i = 0; i < times; ++i)
    {
        AddRequest req;
        req.a = i;
        req.b = i * 2;

        std::cout << "Client sending request: " << req.a << " + " << req.b << std::endl;

        // 发送请求，并指定回调函数处理响应，等待3000毫秒接收回复，过时不再处理回复
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

# 客户端
```c++
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
```
