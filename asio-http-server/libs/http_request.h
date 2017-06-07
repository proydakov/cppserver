#pragma once

#include <map>
#include <string>
#include <istream>
#include <boost/asio/streambuf.hpp>

class http_request
{
public:
    typedef std::map<std::string, std::string>::const_iterator header_iterator;

    http_request()
    {
    }

    size_t parse(boost::asio::streambuf& buffer);

    const std::string& get_method()
    {
        return method_;
    }

    const std::string& get_url()
    {
        return url_;
    }

    const std::string& get_version()
    {
        return version_;
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
    std::string method_;
    std::string url_;
    std::string version_;
    std::map<std::string, std::string> headers_;
};
