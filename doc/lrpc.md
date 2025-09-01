# requirement
参考iceoryx_examples/icehello/的cpp用例，
封装客户端 include/client.h
namespace en_lrpc{ 
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
        std::string m_client_id;

    public:
        client(const std::string file = __FILE__, int line = __LINE__)
        {
            std::hash<std::string> hasher;
            std::string request_name = typeid(Request).name();
            std::string response_name = typeid(Response).name();
            uint64_t client_id = hasher(file + std::to_string(line) + request_name + response_name);
            m_client_id = std::to_string(client_id);
            m_publisher = std::make_shared<publisher_type>({request_name, response_name, request_name});
            m_subscriber = std::make_shared<subscriber_type>({request_name, response_name, m_client_id});
        }

        bool send(const Request &request, std::function<void(const Response &)> callback = nullptr, uint64_t timeout_ms = 1000);
    };
}


封装服务端 include/server.h
namespace en_lrpc{ 
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

    public:
        server(void)
        {
            std::string request_name = typeid(Request).name();
            std::string response_name = typeid(Response).name();
            m_subscriber = std::make_shared<subscriber_type>({request_name, response_name, request_name});
        }

        bool recv(std::function<bool(const Request &, Response&)> callback = nullptr, uint64_t timeout_ms = 1000) 
        {
            if (nullptr == callback) {
                return false;
            }

            if (!m_subscriber) {
                return false;
            }

            auto request_result = m_subscriber.take();
            if (false == request_result.has_value()) {
                return false;
            }

            auto &request = request_result.value();
            std::string client_id = std::to_string(request.client_id);
            auto publisher = std::make_shared<publisher_type>({request_name, response_name, std::to_string(client_id)});
            if (!publisher) {
                return false;
            }

            
            auto response = publisher->loan();
            if (response.has_value()) {
                return false;
            }

            bool ret = callback(request.request, response.value());
            if (false == ret) {
                return false;
            }

            auto publisher = std::make_shared<publisher_type>({request_name, response_name, client_id});
            if (publisher) {
                publisher.send(response);
                return true;
            }

            return true;
        }
    };
}