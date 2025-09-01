#pragma once

#include "iceoryx/iceoryx_posh/popo/publisher.hpp"
#include "iceoryx/iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx/iceoryx_posh/runtime/posh_runtime.hpp"
#include "iceoryx/iceoryx_posh/capro/service_description.hpp"

#include <string>
#include <functional>
#include <memory>
#include <typeinfo>
#include <chrono>
#include <thread>
#include <iostream>

#include "iceoryx/iox/string.hpp"

namespace iox_lrpc {
template<typename Request, typename Response>
class client
{
private:
    struct RequestWrapper {
        uint64_t client_id;
        Request request;
    };

    using publisher_type = iox::popo::Publisher<RequestWrapper>;
    using subscriber_type = iox::popo::Subscriber<Response>;

    std::shared_ptr<publisher_type> m_publisher;
    std::shared_ptr<subscriber_type> m_subscriber;
    std::string m_app_name = "en-lrpc-client";
    uint64_t m_client_id;

public:
    /**
     * @brief 客户端构造函数，通过传入的file,line,app_name生成唯一的客户端ID
     * @param file 客户端文件名,默认传入当前文件名
     * @param line 客户端行号,默认传入当前行号
     * @param app_name 客户端应用名,默认空值时使用默认值"en-lrpc-client"
     */
    client(const std::string& file, int line, const char *app_name=nullptr)
    {
        if (app_name) {
            m_app_name = app_name;
        }

        // Initialize runtime
        iox::runtime::PoshRuntime::initRuntime(
            iox::RuntimeName_t(iox::TruncateToCapacity, m_app_name.c_str())
        );

        std::hash<std::string> hasher;
        std::string request_name = typeid(Request).name();
        std::string response_name = typeid(Response).name();
        m_client_id = hasher(m_app_name + file + std::to_string(line) + request_name + response_name);
        std::string client_id = std::to_string(m_client_id);

        // Create publisher for sending requests
        auto publisher_service_description = iox::capro::ServiceDescription(
            iox::capro::IdString_t(iox::TruncateToCapacity, request_name.c_str()),
            iox::capro::IdString_t(iox::TruncateToCapacity, response_name.c_str()),
            iox::capro::IdString_t(iox::TruncateToCapacity, request_name.c_str())
        );
        m_publisher = std::make_shared<publisher_type>(publisher_service_description);

        // Create subscriber for receiving responses
        auto subscriber_service_description = iox::capro::ServiceDescription(
            iox::capro::IdString_t(iox::TruncateToCapacity, request_name.c_str()),
            iox::capro::IdString_t(iox::TruncateToCapacity, response_name.c_str()),
            iox::capro::IdString_t(iox::TruncateToCapacity, client_id.c_str())
        );
        m_subscriber = std::make_shared<subscriber_type>(subscriber_service_description);
    }

    /**
     * @brief 发送请求并等待响应
     * @param request 请求对象
     * @param callback 响应回调函数
     * @param timeout_ms 超时时间，单位毫秒
     */
    bool send(const Request &request, std::function<void(const Response &)> callback = nullptr, uint64_t timeout_ms = 1000)
    {
        if (!m_publisher) {
            std::cerr << "Publisher not initialized" << std::endl;
            return false;
        }

        // Create request wrapper with client ID
        RequestWrapper request_wrapper;
        request_wrapper.client_id = m_client_id;
        request_wrapper.request = request;

        // Loan memory for the request
        auto loan_result = m_publisher->loan();
        if (!loan_result.has_value()) {
            std::cerr << "Failed to loan memory for request" << std::endl;
            return false;
        }

        // Copy request data to shared memory
        auto& sample = loan_result.value();
        *sample = request_wrapper;
        sample.publish();

        // If no callback provided, just return success
        if (!callback) {
            std::cerr << "No callback provided, returning success" << std::endl;
            return true;
        }

        // Wait for response with timeout
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(timeout_ms)) {
            auto take_result = m_subscriber->take();
            if (take_result.has_value()) {
                callback(*take_result.value());
                return true;
            }
            
            // Sleep a bit to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        std::cerr << "Timeout waiting for response" << std::endl;
        return false;
    }
};
} // namespace iox_lrpc
