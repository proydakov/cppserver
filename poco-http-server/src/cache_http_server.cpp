#include <mutex>
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

class IRequestHandler : public HTTPRequestHandler
{
public:
    void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) override
    {
        resp.setStatus(HTTPResponse::HTTP_OK);
        resp.setContentType("text/plain");

        std::ostream& out = resp.send();

        out << 1024;
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
        parameters->setTimeout(1000);
        parameters->setMaxQueued(1000);
        parameters->setMaxThreads(hardware_concurrency);

        const Poco::UInt16 port = 9999;
        const Poco::Net::ServerSocket socket(port);
        HTTPServer s(new IRequestHandlerFactory, socket, parameters);

        s.start();
        std::cout << "server started: 127.0.0.1:" << port << std::endl;

        waitForTerminationRequest();  // wait for CTRL-C or kill

        std::cout << "shutting down..." << std::endl;
        s.stop();

        return Application::EXIT_OK;
    }
};

int main(int argc, char** argv)
{
    IServerApplication app;
    return app.run(argc, argv);
}
