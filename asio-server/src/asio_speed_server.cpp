#include <memory>
#include <thread>
#include <utility>
#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class session : public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket) :
        socket_(std::move(socket)),
        data_{ 0 }
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
                std::clog << "read from: " << &socket_
                          << " data: " << data_.data()
                          << std::endl;
                if (!ec) {
                    do_write();
                }
        });
    }

    void do_write()
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                //std::clog << "do_write: " << length << std::endl;
                if (!ec) {
                    do_write();
                }
                else {
                    std::clog << "done: " << &socket_ << std::endl;
                }
        });
    }

private:
    tcp::socket socket_;

    enum { max_length = 1024 * 256 };
    std::array<char, max_length> data_;
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

        //std::clog.setstate(std::ios_base::failbit);

        boost::asio::io_service io_service;
        server s(io_service, std::atoi(argv[1]));

        std::clog << "<- io_service run" << std::endl;
        io_service.run();
        std::clog << "<- io_service done" << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "exception: " << e.what() << "\n";
    }

    return 0;
}
