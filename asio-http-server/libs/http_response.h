#pragma once

#include <map>
#include <string>
#include <istream>
#include <boost/asio/streambuf.hpp>

class http_response
{
public:
    typedef std::map<std::string, std::string>::const_iterator header_iterator;

    http_response()
    {
    }

    size_t parse(boost::asio::streambuf& buffer);
    size_t parse(const std::string& str);

    const std::string& get_version()
    {
        return version_;
    }

    size_t get_code()
    {
        return status_code_;
    }

    const std::string& get_message()
    {
        return status_message_;
    }

    const std::string get_header(const std::string& name, const std::string def = "")
    {
        auto it = headers_.find(name);
        if(it != headers_.end()) {
            return it->second;
        }
        return def;
    }

    header_iterator begin()
    {
        return headers_.cbegin();
    }

    header_iterator end()
    {
        return headers_.end();
    }

private:
    template<class cstream>
    void http_response_from_stream(cstream& stream);

private:
    std::string version_;
    size_t      status_code_;
    std::string status_message_;
    std::map<std::string, std::string> headers_;
};
