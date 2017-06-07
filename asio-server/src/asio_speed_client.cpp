#include <string>
#include <istream>
#include <ostream>
#include <iostream>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

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
        strand_(io_service)
    {
        std::clog << "<- client" << std::endl;
    }

    ~client()
    {
        std::clog << "<- ~client" << std::endl;
    }

    void go(const std::string& server, int port)
    {
        std::clog << "<- go" << std::endl;

        auto self(shared_from_this());

        boost::system::error_code err;

        tcp::endpoint endpoint( boost::asio::ip::address::from_string(server), port );

        std::clog << "<- schedule async_connect" << std::endl;
        socket_.async_connect(endpoint,
            [this, self](boost::system::error_code ec) {
                if (!ec) {
                    do_write();
                }
        });
    }

private:
    void do_write()
    {
        auto self(shared_from_this());
        build_request();
        boost::asio::async_write(socket_, request_,
            [this, self](boost::system::error_code ec, std::size_t length) {
                //std::clog << "do_write: " << length << std::endl;
                if (!ec) {
                    do_read();
                }
        });
    }

    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(response_buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    do_read();
                }
        });
    }

    void build_request()
    {
        std::ostream request_stream(&request_);
        request_stream << "GET";
    }

private:
    tcp::socket socket_;
    boost::asio::io_service::strand strand_;

    boost::asio::streambuf request_;

    enum { max_length = 1024 * 256 };
    std::array<char, max_length> response_buffer_;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3) {
            std::cout << "Usage: async_speed_client <server> <port>\n";
            return 1;
        }

        //std::clog.setstate(std::ios_base::failbit);

        boost::asio::io_service io_service;
        auto c = std::make_shared<client>(io_service);
        c->go( argv[1], std::stoi(argv[2]) );
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
