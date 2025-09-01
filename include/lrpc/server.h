#ifndef EN_LRPC_SERVER_H
#define EN_LRPC_SERVER_H

#include "iceoryx/iceoryx_posh/popo/publisher.hpp"
#include "iceoryx/iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx/iceoryx_posh/runtime/posh_runtime.hpp"

#include <string>
#include <functional>
#include <memory>
#include <typeinfo>
#include <chrono>
#include <thread>
#include <iostream>

namespace en_lrpc {

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

public:
    server()
    {
        // Initialize runtime
        constexpr char APP_NAME[] = "en-lrpc-server";
        iox::runtime::PoshRuntime::initRuntime(APP_NAME);

        m_request_name = typeid(Request).name();
        m_response_name = typeid(Response).name();

        // Create subscriber for receiving requests
        m_subscriber = std::make_shared<subscriber_type>(
            iox::popo::SubscriberOptions{}.withServiceDescription({m_request_name.c_str(), m_response_name.c_str(), m_request_name.c_str()})
        );
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
                
                // Create publisher for sending response to specific client
                std::string client_id = std::to_string(request_wrapper.client_id);
                auto publisher = std::make_shared<publisher_type>(
                    iox::popo::PublisherOptions{}.withServiceDescription({m_request_name.c_str(), m_response_name.c_str(), client_id.c_str()})
                );

                if (!publisher) {
                    std::cerr << "Failed to create publisher for client: " << client_id << std::endl;
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
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::cerr << "Timeout waiting for request" << std::endl;
        return false;
    }
};

} // namespace en_lrpc

#endif // EN_LRPC_SERVER_H
