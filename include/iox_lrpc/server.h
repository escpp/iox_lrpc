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

    std::shared_ptr<subscriber_type> m_subscriber;
    std::string m_request_name;
    std::string m_response_name;
    std::unordered_map<uint64_t, std::shared_ptr<publisher_type>> m_clients;
    std::mutex m_clients_mutex;

public:
    server()
    {
        // Initialize runtime
        constexpr char APP_NAME[] = "en-lrpc-server";
        iox::runtime::PoshRuntime::initRuntime(APP_NAME);

        m_request_name = typeid(Request).name();
        m_response_name = typeid(Response).name();

        // Create subscriber for receiving requests
        auto service_description = iox::capro::ServiceDescription(
            iox::capro::IdString_t(iox::TruncateToCapacity, m_request_name.c_str()),
            iox::capro::IdString_t(iox::TruncateToCapacity, m_response_name.c_str()),
            iox::capro::IdString_t(iox::TruncateToCapacity, m_request_name.c_str())
        );
        m_subscriber = std::make_shared<subscriber_type>(service_description);
    }

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
                std::shared_ptr<publisher_type> publisher;
                {
                    std::lock_guard<std::mutex> lock(m_clients_mutex);
                    auto it = m_clients.find(request_wrapper.client_id);
                    if (it != m_clients.end()) {
                        publisher = it->second;
                    } else {
                        std::string client_id_str = std::to_string(request_wrapper.client_id);
                        auto publisher_service_description = iox::capro::ServiceDescription(
                            iox::capro::IdString_t(iox::TruncateToCapacity, m_request_name.c_str()),
                            iox::capro::IdString_t(iox::TruncateToCapacity, m_response_name.c_str()),
                            iox::capro::IdString_t(iox::TruncateToCapacity, client_id_str.c_str())
                        );
                        publisher = std::make_shared<publisher_type>(publisher_service_description);
                        if (!publisher) {
                            std::cerr << "Failed to create publisher for client: " << client_id_str << std::endl;
                            return false;
                        }
                        m_clients[request_wrapper.client_id] = publisher;
                    }
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
