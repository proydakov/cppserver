#include <memory>
#include <thread>
#include <utility>
#include <cstdlib>
#include <iostream>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

const std::string RESPONSE =
R"(HTTP/1.1 200 OK
Content-Type: application/json
Server: ashttp
Date: Wed, 07 Jun 2017 16:19:01 GMT
Content-Length: 95)" + std::string("\r\n\r\n") +

R"({"Hello":"world","T":true,"F":false,"N":null,"I":123,"PI":3.1416,"Array":[0,1,2,3,4,5,6,7,8,9]})";

class session : public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket) : socket_(std::move(socket))
    {
    }

    ~session()
    {
        //std::cout << "~session" << std::endl;
    }

    void start()
    {
        do_read();
    }

private:
    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    do_write(length);
                }
        });
    }

    void do_write(std::size_t length)
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(RESPONSE),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                return;
        });
  }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class server
{
public:
    server(boost::asio::io_service& io_service, short port) :
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
        socket_(io_service)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec) {
                std::make_shared<session>(std::move(socket_))->start();
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;
        server s(io_service, std::atoi(argv[1]));

        std::vector<std::thread> threads;
        const size_t hardware_concurrency = std::thread::hardware_concurrency();
        for(size_t i = 0; i < hardware_concurrency; i++) {
            threads.push_back(std::thread([&io_service](){
                io_service.run();
            }));
        }
        for(size_t i = 0; i < hardware_concurrency; i++) {
            threads[i].join();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
