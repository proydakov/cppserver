#include "http_request.h"

#include <boost/asio/streambuf.hpp>

size_t http_request::parse(boost::asio::streambuf& buffer)
{
    std::istream response_stream(&buffer);

    response_stream >> method_;
    response_stream >> url_;
    response_stream >> version_;

    std::string key;
    std::string value;
    std::string header;

    std::getline(response_stream, header);
    while (std::getline(response_stream, header) && header != "\r") {
        header.resize(header.size() - 1);
        std::size_t found = header.find(':');
        if (found != std::string::npos) {
            key = header.substr(0, found);
            value = header.substr(found + 2);
            headers_[key] = value;
        }
    }

    return buffer.size();
}
