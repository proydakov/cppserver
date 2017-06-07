#include <thread>
#include <string>
#include <vector>
#include <iostream>

#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>

using namespace Poco::Net;
using namespace Poco::Util;

namespace {

const std::string PAGE = R"(
<!DOCTYPE html>
<html>
<head>
<title>Welcome to POCO!</title>
<style>
    body {
        width: 35em;
        margin: 0 auto;
        font-family: Tahoma, Verdana, Arial, sans-serif;
    }
</style>
</head>
<body>
<h1>Welcome to POCO!</h1>
<p>If you see this page, the poco web server is successfully compiled and
working.</p>

<p>For online documentation please refer to
<a href="https://github.com/proydakov/poco-http-server">poco-http-server</a>.<br/>

<p><em>Thank you for using POCO.</em></p>
</body>
</html>
)";

}

class IRequestHandler : public HTTPRequestHandler
{
public:
    void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) override
    {
        std::vector<int> data(1024 * 1024 * 16, 0);

        resp.setStatus(HTTPResponse::HTTP_OK);
        resp.setContentType("text/html");

        std::ostream& out = resp.send();

        out << "vec[" << data.size() << "]";
        out.flush();
    }
};

class IRequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest &) override
    {
        return new IRequestHandler;
    }
};

class IServerApplication : public ServerApplication
{
protected:
    int main(const std::vector<std::string>& args)
    {
        unsigned int hardware_concurrency = std::thread::hardware_concurrency();

        Poco::Net::HTTPServerParams::Ptr parameters = new Poco::Net::HTTPServerParams();
        parameters->setTimeout(10000);
        parameters->setMaxQueued(1000);
        parameters->setMaxThreads(hardware_concurrency);

        const Poco::UInt16 port = 11111;
        const Poco::Net::ServerSocket socket(port);
        HTTPServer s(new IRequestHandlerFactory, socket, parameters);

        s.start();
        std::cout << "server started: 127.0.0.1:" << port << std::endl;

        waitForTerminationRequest();  // wait for CTRL-C or kill

        std::cout << "shutting down... " << std::endl;
        s.stop();

        return Application::EXIT_OK;
    }
};

int main(int argc, char** argv)
{
    std::vector<int> data(1024 * 1024 * 32, 0);

    IServerApplication app;

    std::cout << "size: " << data.size() << std::endl;

    return app.run(argc, argv);
}
