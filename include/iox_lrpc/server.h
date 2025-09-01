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
#include <unordered_map>
#include <mutex>

#include "iceoryx/iox/string.hpp"

namespace iox_lrpc {

template<typename Request, typename Response>
class server
{
private:
    struct RequestWrapper {
        uint64_t client_id;
        Request request;
    };

    using subscriber_type = iox::popo::Subscriber<RequestWrapper>;
    using publisher_type = iox::popo::Publisher<Response>;

    /**
     * @brief 客户端管理器，用于管理客户端的连接和消息发布
     */
    class ClientManager {
    private:
        std::unordered_map<uint64_t, std::shared_ptr<publisher_type>> m_client_manager;
        std::mutex m_clients_mutex;

    public:
        /**
         * @brief 向客户端管理器中添加一个客户端
         * @param client_id 客户端ID
         * @param publisher 客户端的发布者
         * @return true 添加成功, false 添加失败
         */
        bool add_client(uint64_t client_id, std::shared_ptr<publisher_type> publisher) 
        {
            std::lock_guard<std::mutex> lock(m_clients_mutex);
            m_client_manager[client_id] = publisher;
            return true;
        }

        /**
         * @brief 向客户端管理器中添加一个客户端
         * @param rq_name 请求名称
         * @param rs_name 响应名称
         * @param id 客户端ID
         * @return std::shared_ptr<publisher_type> 客户端的发布者, nullptr 表示添加失败
         */
        std::shared_ptr<publisher_type> add_client(const std::string& rq_name, const std::string &rs_name, uint64_t id) 
        {
            std::string id_string = std::to_string(id);
            auto publisher_service_description = iox::capro::ServiceDescription(
                iox::capro::IdString_t(iox::TruncateToCapacity, rq_name.c_str()),
                iox::capro::IdString_t(iox::TruncateToCapacity, rs_name.c_str()),
                iox::capro::IdString_t(iox::TruncateToCapacity, id_string.c_str())
            );

            std::shared_ptr<publisher_type> publisher = std::make_shared<publisher_type>(publisher_service_description);
            if (!publisher) {
                std::cerr << "Failed to create publisher for client: " << id << std::endl;
                return nullptr;
            }

            if (add_client(id, publisher)) {
                return publisher;
            }

            return nullptr;
        }

        /**
         * @brief 从客户端管理器中移除一个客户端
         * @param client_id 客户端ID
         */
        void remove_client(uint64_t client_id) 
        {
            std::lock_guard<std::mutex> lock(m_clients_mutex);
            m_client_manager.erase(client_id);
        }

        /**
         * @brief 获取客户端的发布者
         * @param client_id 客户端ID
         * @return std::shared_ptr<publisher_type> 客户端的发布者, nullptr 表示获取失败
         */
        std::shared_ptr<publisher_type> get_publisher(uint64_t client_id) 
        {
            std::lock_guard<std::mutex> lock(m_clients_mutex);
            auto it = m_client_manager.find(client_id);
            if (it != m_client_manager.end()) {
                return it->second;
            } 
            return nullptr;
        }
    };

    std::shared_ptr<subscriber_type> m_subscriber;
    std::string m_app_name;
    std::string m_request_name;
    std::string m_response_name;
    ClientManager m_client_manager;

public:
    /**
     * @brief iox_lrpc服务器构造函数，通过模板参数确定唯一应用名，可以接受多个客户端连接
     */
    server(void)
    {
        m_request_name = typeid(Request).name();
        m_response_name = typeid(Response).name();
        m_app_name = m_request_name + m_response_name;

        // Initialize runtime
        iox::runtime::PoshRuntime::initRuntime(
            iox::RuntimeName_t(iox::TruncateToCapacity, m_app_name.c_str())
        );


        // Create subscriber for receiving requests
        auto service_description = iox::capro::ServiceDescription(
            iox::capro::IdString_t(iox::TruncateToCapacity, m_request_name.c_str()),
            iox::capro::IdString_t(iox::TruncateToCapacity, m_response_name.c_str()),
            iox::capro::IdString_t(iox::TruncateToCapacity, m_request_name.c_str())
        );
        m_subscriber = std::make_shared<subscriber_type>(service_description);
    }

    /**
     * @brief 接收请求并处理
     * @param callback 请求处理回调函数
     * @param timeout_ms 超时时间，单位毫秒
     * @return true 处理成功, false 处理失败
     */
    bool recv(std::function<bool(const Request &, Response&)> callback = nullptr, uint64_t timeout_ms = 1000) 
    {
        if (nullptr == callback) {
            std::cerr << "Callback function is required" << std::endl;
            return false;
        }

        if (!m_subscriber) {
            std::cerr << "Subscriber not initialized" << std::endl;
            return false;
        }

        // Wait for request with timeout
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start_time < std::chrono::milliseconds(timeout_ms)) {
            auto request_result = m_subscriber->take();
            if (request_result.has_value()) {
                auto& request_wrapper = *request_result.value();
                
                // Get or create publisher for sending response to specific client
                std::shared_ptr<publisher_type> publisher = m_client_manager.get_publisher(request_wrapper.client_id);
                if (nullptr == publisher) {
                    publisher = m_client_manager.add_client(m_request_name, m_response_name, request_wrapper.client_id);
                }

                if (nullptr == publisher) {
                    std::cerr << "Failed to get or create publisher for client: " << request_wrapper.client_id << std::endl;
                    return false;
                }

                // Loan memory for the response
                auto loan_result = publisher->loan();
                if (!loan_result.has_value()) {
                    std::cerr << "Failed to loan memory for response" << std::endl;
                    return false;
                }

                // Process the request and prepare response
                auto& response_sample = loan_result.value();
                bool ret = callback(request_wrapper.request, *response_sample);
                if (!ret) {
                    std::cerr << "Callback returned false" << std::endl;
                    return false;
                }

                // Send the response
                response_sample.publish();
                return true;
            }
            
            // Sleep a bit to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        std::cerr << "Timeout waiting for request" << std::endl;
        return false;
    }
};
} // namespace iox_lrpc
