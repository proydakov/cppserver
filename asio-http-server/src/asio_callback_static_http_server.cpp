//
// asio_callback_http_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2016 Evgeny M. Proydakov (e.proydakov dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <memory>
#include <thread>
#include <utility>
#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

const std::string RESPONSE =
R"(HTTP/1.1 200 OK
Date: Thu, 08 Dec 2016 00:53:42 GMT
Connection: Close
Content-Type: text/html


<!DOCTYPE html>
<html>
<head>
<title>Welcome to ASIO!</title>
<style>
    body {
        width: 35em;
        margin: 0 auto;
        font-family: Tahoma, Verdana, Arial, sans-serif;
    }
</style>
</head>
<body>
<h1>Welcome to ASIO!</h1>
<p>If you see this page, the asio web server is successfully compiled and
working.</p>

<p>For online documentation please refer to
<a href="https://github.com/proydakov/asio-http-server">asio-http-server</a>.<br/>

<p><em>Thank you for using ASIO.</em></p>
</body>
</html>
)";

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
