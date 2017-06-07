//
// asio_spawn_proxy_http_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2016 Evgeny M. Proydakov (e.proydakov dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <list>
#include <cmath>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include <memory>
#include <sstream>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>

#include <http_request.h>
#include <http_response.h>

using boost::asio::ip::tcp;

namespace {
    const std::size_t TIMEOUT = 1000;

    const std::string SERVER_HOST = "nginx.org";
    const std::string SERVER_PATH = "/";
    const std::string SERVER_ADDR = "95.211.80.227";
    const std::size_t SERVER_PORT = 80;
}

class timeout_exception : public std::exception
{
public:
    const char* what() const throw() override
    {
        return "timeout"; // my error message
    }
};

template<class T1>
void check_error(T1 error)
{
    if(error) {
        std::stringstream sstream;
        sstream << error;
        throw std::runtime_error(sstream.str());
    }
}

template<class T1, class T2>
void check_error_and_timeout(T1 error, T2& timeout)
{
    if(error) {
        std::stringstream sstream;
        sstream << error;
        throw std::runtime_error(sstream.str());
    }
    if(timeout) {
        throw timeout_exception();
    }
}

///////////////////////////////////////////////////////////////////////////////
//------------------------------- client --------------------------------------
///////////////////////////////////////////////////////////////////////////////

namespace {
    std::atomic<size_t> client_counter(0);
    std::atomic<size_t> client_sequence(0);
}

class client : public std::enable_shared_from_this<client>
{
public:
    client(boost::asio::io_service& io_service) :
        socket_(io_service),
        timer_(io_service),
        timer_counter_(0),
        timeout_(false)
    {
        client_counter++;
        sequence_ = client_sequence++;
        std::clog << "<- " << sequence_ << " client" << std::endl;
    }

    ~client()
    {
        client_counter--;
        std::clog << "<- " << sequence_ << " ~client" << std::endl;
    }

    bool go(const std::string& hostname, const std::string& path,
            const std::string& server, int port,
            boost::asio::io_service::strand& strand,
            boost::asio::yield_context& yield)
    {
        std::clog << "<- " << sequence_ << " go client" << std::endl;

        bool error = false;
        http_response response;

        try {
            boost::system::error_code err;

            std::clog << "<- " << sequence_ << " schedule timer" << std::endl;
            timer_.expires_from_now(std::chrono::milliseconds(TIMEOUT));
            schedule_timer(strand);

            if(!socket_.is_open()) {
                std::clog << "<- " << sequence_ << " schedule async_connect" << std::endl;
                boost::asio::ip::tcp::endpoint endpoint( boost::asio::ip::address::from_string(server), port );
                socket_.async_connect(endpoint, yield[err]);
                check_error_and_timeout(err, timeout_);

                tcp::socket::reuse_address ra(true);
                tcp::socket::keep_alive ka(true);

                socket_.set_option(ra);
                socket_.set_option(ka);
            }

            build_request(hostname, path, port);

            std::clog << "<- " << sequence_ << " schedule async_write" << std::endl;
            boost::asio::async_write(socket_, request_, yield[err]);
            check_error_and_timeout(err, timeout_);

            std::clog << "<- " << sequence_ << " schedule async_read_until head" << std::endl;
            boost::asio::async_read_until(socket_, response_, "\r\n\r\n", yield[err]);
            check_error_and_timeout(err, timeout_);

            std::string str_buff = buffer_to_string(response_);
            const size_t body_size = response.parse(str_buff);
            dump_response(response);

            const std::string str_content_length = response.get_header("Content-Length", "");
            const size_t content_length = std::stoi(str_content_length);
            if(!str_content_length.empty() && content_length - body_size) {
                std::clog << "<- " << sequence_ << " schedule async_read body" << std::endl;
                boost::asio::async_read(socket_, response_,
                    boost::asio::transfer_at_least(content_length - body_size),
                    yield[err]);
                check_error_and_timeout(err, timeout_);
            }
        }
        catch (const timeout_exception& e) {
            error = true;
            std::clog << "<- " << sequence_ << " timeout error: " << e.what() << std::endl;
        }
        catch (const std::exception& e) {
            error = true;
            socket_.close();
            std::clog << "<- " << sequence_ << " catch error: " << e.what() << std::endl;
        }
        catch (...) {
            error = true;
            socket_.close();
            std::clog << "<- " << sequence_ << " unknown error" << std::endl;
        }
        std::clog << "<- " << sequence_ << " cancle timer" << std::endl;

        timer_.cancel();
        if(error || (response.get_header("Connection", "") != "keep-alive")) {
            std::clog << "<- !!!!! " << sequence_ << " close keep-alive" << std::endl;
            socket_.close();
        }
        std::clog << "<- " << sequence_ << " done" << std::endl;

        return error;
    }

    boost::asio::streambuf& get_response()
    {
        return response_;
    }

private:
    void build_request(const std::string& hostname, const std::string& path, int port)
    {
        std::ostream request_stream(&request_);
        request_stream << "GET " << path << " HTTP/1.1\r\n";
        request_stream << "Host: " << hostname;
        if(port != 80) {
            request_stream << ":" << port;
        }
        request_stream << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "User-Agent: asio/1.60.0\r\n";
        request_stream << "Connection: keep-alive\r\n";
        request_stream << "\r\n";
    }

    void schedule_timer(boost::asio::io_service::strand& strand)
    {
        timeout_ = false;

        size_t value = timer_counter_++;

        auto self(shared_from_this());
        boost::asio::spawn(strand, [this, self, value](boost::asio::yield_context yield) {
            bool done = false;
            while (!done) {
                boost::system::error_code ec;
                timer_.async_wait(yield[ec]);
                std::clog << "<- " << sequence_ << " timer gotcha: " << value << " err: " << ec << std::endl;
                if(boost::asio::error::operation_aborted == ec) {
                    done = true;
                }
                else {
                    if (timer_.expires_from_now() <= std::chrono::seconds(0)) {
                        self->timeout_ = true;
                        done = true;
                        std::clog << "<- " << sequence_ << " timer timeout: " << value << std::endl;
                    }
                }
            }
        });
    }

    void dump_response(http_response& response)
    {
        std::clog << ">"
                  << " " << response.get_version()
                  << " " << response.get_code()
                  << " " << response.get_message()
                  << "\n";

        for(auto it = response.begin(); it != response.end(); ++it) {
            std::clog << "> " << it->first << ": " << it->second << "\n";
        }
        std::clog << ">" << std::endl;
    }

    std::string buffer_to_string(const boost::asio::streambuf& buffer)
    {
        using boost::asio::buffers_begin;
        auto bufs = buffer.data();
        std::string result(buffers_begin(bufs), buffers_begin(bufs) + buffer.size());
        return result;
    }

private:
    size_t sequence_;

    tcp::socket socket_;
    boost::asio::steady_timer timer_;

    boost::asio::streambuf request_;
    boost::asio::streambuf response_;

    size_t timer_counter_;
    std::atomic<bool> timeout_;
};

///////////////////////////////////////////////////////////////////////////////
//--------------------------- client_pool -------------------------------------
///////////////////////////////////////////////////////////////////////////////

class client_pool
{
public:
    client_pool(boost::asio::io_service& io_service) :
        io_service_(io_service)
    {
    }

    std::shared_ptr<client> get_client()
    {
        {
            std::lock_guard<std::mutex> guard(mutex_);
            if(!clients_.empty()) {
                auto client = clients_.front();
                clients_.pop_front();
                return client;
            }
        }
        auto c = std::make_shared<client>(io_service_);
        return c;
    }

    void return_client(const std::shared_ptr<client>& c)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        clients_.push_back(c);
    }

private:
    boost::asio::io_service& io_service_;

    std::mutex mutex_;
    std::list<std::shared_ptr<client>> clients_;
};

///////////////////////////////////////////////////////////////////////////////
//------------------------------- session -------------------------------------
///////////////////////////////////////////////////////////////////////////////

namespace {
    std::atomic<size_t> session_counter(0);
    std::atomic<size_t> session_sequence(0);

    const size_t MAX_SESSIONS = 10000;
}

class session : public std::enable_shared_from_this<session>
{
public:
    explicit session(tcp::socket socket, client_pool& pool) :
        socket_(std::move(socket)),
        strand_(socket_.get_io_service()),
        pool_(pool)
    {
        boost::asio::ip::tcp::socket::reuse_address ra(true);
        boost::asio::ip::tcp::socket::keep_alive ka(true);
        socket_.set_option(ra);
        socket_.set_option(ka);

        counter_ = session_counter++;
        sequence_ = session_sequence++;
        std::clog << "-> " << sequence_ << " session " << std::endl;
    }

    ~session()
    {
        socket_.close();
        std::clog << "-> " << sequence_ << " ~session" << sequence_ << std::endl;
        session_counter--;
    }

    void go()
    {
        std::clog << "-> " << sequence_ << " go session" << std::endl;

        if(counter_ > MAX_SESSIONS) {
            std::cerr << "-> " << sequence_ << " FORCE CLOSE" << std::endl;
            socket_.close();
            return;
        }

        auto self(shared_from_this());
        boost::asio::spawn(strand_, [this, self](boost::asio::yield_context yield) {
            try {
                bool close = false;
                for(size_t i = 1; !close; i++) {
                    boost::system::error_code err;

                    std::clog << "-> " << sequence_ << " schedule read: " << i << std::endl;

                    boost::asio::async_read_until(socket_, request_, "\r\n\r\n", yield[err]);
                    check_error(err);

                    http_request request;
                    request.parse(request_);
/*
                    std::clog << "@request method:  '" << request.get_method() << "'" << std::endl;
                    std::clog << "@request url:     '" << request.get_url() << "'" << std::endl;
                    std::clog << "@request version: '" << request.get_version() << "'" << std::endl;
                    std::clog << "@request headers:  " << std::endl;
                    for(auto it = request.begin(); it != request.end(); ++it) {
                        std::clog << "'" << it->first << "': '" << it->second << "'" << std::endl;
                    }
*/
                    std::clog << "-> " << sequence_ << " read: " << request_.size() << " in: " << std::this_thread::get_id() << std::endl;
                    request_.consume(request_.size());

                    auto c = pool_.get_client();
                    bool error = c->go(SERVER_HOST, SERVER_PATH, SERVER_ADDR, SERVER_PORT, strand_, yield);
                    pool_.return_client(c);

                    if(error) {
                        build_response();
                        std::clog << "-> " << sequence_ << " schedule write" << std::endl;
                        auto& buff = response_;
                        const size_t size = buff.size();
                        boost::asio::async_write(socket_, buff, yield[err]);
                        check_error(err);
                        std::clog << "-> " << sequence_ << " write: " << size << " in: " << std::this_thread::get_id() << std::endl;
                    }
                    else {
                        std::clog << "-> " << sequence_ << " schedule write" << std::endl;
                        auto& buff = c->get_response();
                        const size_t size = buff.size();
                        boost::asio::async_write(socket_, buff, yield[err]);
                        check_error(err);
                        std::clog << "-> " << sequence_ << " write: " << size << " in: " << std::this_thread::get_id() << std::endl;
                    }
                    if(request.get_header("Connection", "") != "keep-alive") {
                        std::clog << "-> " << sequence_ << " close keep-alive" << std::endl;
                        close = true;
                    }
                }
            }
            catch (const std::exception& e) {
                std::clog << "-> " << sequence_ << " catch error: " << e.what() << std::endl;
            }
            catch (...) {
                std::clog << "-> " << sequence_ << " unknown error" << std::endl;
            }

            std::clog << "-> " << sequence_ << " done" << std::endl;
        });
    }

private:
    void build_response()
    {
        std::ostream stream(&response_);
        stream << "HTTP/1.1 500 Internal Server Error\r\n";
        stream << "\r\n";
    }

private:
    size_t counter_;
    size_t sequence_;

    tcp::socket socket_;
    boost::asio::io_service::strand strand_;
    client_pool& pool_;

    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <port>\n";
            return 1;
        }
        std::cerr << "#> starting: " << argv[0] << ":" << argv[1] << std::endl;
        std::clog.setstate(std::ios_base::failbit);

        boost::asio::io_service io_service;
        boost::asio::io_service::strand io_strand(io_service);

        bool done = false;
        boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code&, const int&){
            std::cerr << "#> catch signal" << std::endl;
            done = true;
            io_service.stop();
        });

        client_pool pool(io_service);

        boost::asio::spawn(io_strand, [&](boost::asio::yield_context yield) {
            tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), std::atoi(argv[1])));

            for (;;) {
                boost::system::error_code ec;
                tcp::socket socket(io_service);
                acceptor.async_accept(socket, yield[ec]);
                if (!ec) std::make_shared<session>(std::move(socket), pool)->go();
            }
        });

        std::vector<std::thread> threads;
        size_t hardware_concurrency = std::thread::hardware_concurrency();
        for(size_t i = 0; i < hardware_concurrency; i++) {
            threads.push_back(std::thread([&io_service](){
                try {
                    io_service.run();
                }
                catch(const std::exception& e) {
                    std::cerr << "#> worker exception: " << e.what() << std::endl;
                }
                catch(...) {
                    std::cerr << "#> worker unknown error" << std::endl;
                }
            }));
        }

        while(!done) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            std::cout << "#>"
                      << " session_counter: " << session_counter
                      << " session_sequence: " << session_sequence
                      << " client_counter: " << client_counter
                      << " client_sequence: " << client_sequence
                      << std::endl;

        }

        for(size_t i = 0; i < threads.size(); i++) {
            threads[i].join();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "#> main exception: " << e.what() << std::endl;;
    }
    catch (...)
    {
        std::cerr << "#> main unknown error" << std::endl;
    }
    return 0;
}
