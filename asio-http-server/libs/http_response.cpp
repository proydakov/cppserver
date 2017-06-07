#include "http_response.h"

#include <sstream>
#include <boost/asio/streambuf.hpp>

size_t http_response::parse(boost::asio::streambuf& buffer)
{
    std::istream istream(&buffer);
    http_response_from_stream(istream);
    return buffer.size();
}

size_t http_response::parse(const std::string& str)
{
    boost::asio::streambuf buffer;

    std::ostream ostream(&buffer);
    ostream << str;

    std::istream istream(&buffer);
    http_response_from_stream(istream);

    return buffer.size();
}

template<class cstream>
void http_response::http_response_from_stream(cstream& stream)
{
    stream >> version_;
    stream >> status_code_;
    stream >> status_message_;

    std::string key;
    std::string value;
    std::string header;

    std::getline(stream, header);
    while (std::getline(stream, header) && header != "\r") {
        header.resize(header.size() - 1);
        std::size_t found = header.find(':');
        if (found != std::string::npos) {
            key = header.substr(0, found);
            value = header.substr(found + 2);
            headers_[key] = value;
        }
    }
}
