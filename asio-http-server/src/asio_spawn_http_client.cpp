//
// asio_spawn_http_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2016 Evgeny M. Proydakov (e.proydakov dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <string>
#include <istream>
#include <ostream>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include <http_request.h>
#include <http_response.h>

using boost::asio::ip::tcp;

namespace {
    const std::size_t TIMEOUT = 5000;
}

template<class T>
void check_error(T error)
{
    if(error) {
        std::stringstream sstream;
        sstream << error;
        throw std::runtime_error(sstream.str());
    }
}

class client : public std::enable_shared_from_this<client>
{
public:
    client(boost::asio::io_service& io_service) :
        socket_(io_service),
        timer_(io_service),
        strand_(io_service)
    {
        std::clog << "<- client" << std::endl;
    }

    ~client()
    {
        std::clog << "<- ~client" << std::endl;
    }

    void go(const std::string& hostname, const std::string& path, const std::string& server, int port)
    {
        std::clog << "<- go" << std::endl;

        auto self(shared_from_this());
        boost::asio::spawn(strand_, [this, self, hostname, path, server, port](boost::asio::yield_context yield) {
            try {
                boost::system::error_code err;

                tcp::endpoint endpoint( boost::asio::ip::address::from_string(server), port );

                std::clog << "<- schedule timer" << std::endl;
                timer_.expires_from_now(std::chrono::milliseconds(TIMEOUT));

                for(size_t i = 0; i < 10000; i++) {
                    std::clog << "<- #################### cycle: " << i + 1 << " ####################" << std::endl;

                    if(!socket_.is_open()) {
                        std::clog << "<- schedule async_connect" << std::endl;
                        socket_.async_connect(endpoint, yield[err]);
                        check_error(err);
                    }

                    build_request(hostname, path, port);
                    dump_request(request_);

                    build_request(hostname, path, port);
                    std::clog << "<- schedule async_write " << request_.size() << std::endl;
                    boost::asio::async_write(socket_, request_, yield[err]);
                    check_error(err);

                    std::clog << "<- schedule async_read_until" << std::endl;
                    boost::asio::async_read_until(socket_, response_, "\r\n\r\n", yield[err]);
                    check_error(err);

                    http_response response;
                    const size_t body_size = response.parse(response_);
                    dump_response(response);

                    if(500 == response.get_code()) {
                        break;
                    }

                    const std::string str_content_length = response.get_header("Content-Length", "");
                    const size_t content_length = std::stoi(str_content_length);
                    if(!str_content_length.empty() && content_length - body_size) {
                        std::clog << "<- schedule async_read body" << std::endl;
                        boost::asio::async_read(socket_, response_,
                            boost::asio::transfer_at_least(content_length - body_size),
                            yield[err]);
                        check_error(err);
                    }

                    if(response_.size()) {
                        std::clog << &response_;
                    }
                }
            }
            catch (const std::exception& e) {
                std::clog << "<- catch error: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::clog << "<- unknown error" << std::endl;
            }
            std::clog << "<- cancle timer" << std::endl;
            timer_.cancel();

            std::clog << "<- done" << std::endl;
            socket_.close();
        });

        boost::asio::spawn(strand_, [this, self](boost::asio::yield_context yield) {
            while (socket_.is_open()) {
                boost::system::error_code ec;
                timer_.async_wait(yield[ec]);
                std::clog << "<- timer gotcha: " << ec << std::endl;
                if(boost::asio::error::operation_aborted == ec) {
                    break;
                }
                if (timer_.expires_from_now() <= std::chrono::seconds(0)) {
                    std::clog << "<- timer timeout" << std::endl;
                    socket_.close();
                }
            }
        });
    }

private:
    void build_request(const std::string& hostname, const std::string& path, int port)
    {
        // Form the request. We specify the "Connection: close" header so that the
        // server will close the socket after transmitting the response. This will
        // allow us to treat all data up until the EOF as the content.
        std::ostream request_stream(&request_);
        request_stream << "GET " << path << " HTTP/1.1\r\n";
        request_stream << "Host: " << hostname;
        if (port != 80) {
            request_stream << ":" << port;
        }
        request_stream << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "User-Agent: asio/1.60.0\r\n";
        request_stream << "Connection: keep-alive\r\n";
        request_stream << "\r\n";
    }

    void dump_request(boost::asio::streambuf& request)
    {
        std::istream response_stream(&request);

        std::string line;
        while (std::getline(response_stream, line) && line != "\r") {
            std::cout << "< " << line << std::endl;
        }
        std::cout << "< " << std::endl;
    }

    void dump_response(boost::asio::streambuf& response)
    {
        std::istream response_stream(&response);

        std::string line;
        while (std::getline(response_stream, line) && line != "\r") {
            std::cout << "> " << line << std::endl;
        }
        std::cout << "> " << std::endl;
    }

    void dump_response(http_response& response)
    {
        std::cout << "> "
                  << response.get_version() << " "
                  << response.get_code() << " "
                  << response.get_message()
                  << "\n";

        for(auto it = response.begin(); it != response.end(); ++it) {
            std::cout << "> " << it->first << ": " << it->second << "\n";
        }
        std::cout << ">" << "\n";
    }

private:
    tcp::socket socket_;
    boost::asio::steady_timer timer_;
    boost::asio::io_service::strand strand_;

    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 5) {
            std::cout << "Usage: async_client <hostname> <path> <server> <port>\n";
            return 1;
        }

        //std::clog.setstate(std::ios_base::failbit);

        boost::asio::io_service io_service;
        auto c = std::make_shared<client>(io_service);
        c->go( argv[1], argv[2], argv[3], std::stoi(argv[4]) );
        c.reset();

        std::clog << "<- io_service run" << std::endl;

        io_service.run();

        std::clog << "<- io_service done" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "<- main exception: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "<- main unknown error" << std::endl;
    }

    return 0;
}
