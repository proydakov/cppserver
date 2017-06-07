#include <thread>
#include <string>
#include <vector>
#include <iterator>
#include <iostream>

#include <Poco/URI.h>
#include <Poco/StreamCopier.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Util/ServerApplication.h>

using Poco::URI;
using Poco::StreamCopier;
using namespace Poco::Net;
using namespace Poco::Util;

namespace {
    const std::size_t TIMEOUT_MICROSECONDS = 150 * 1000;
    const std::size_t MAX_QUEUE = 1000;

    const std::string SERVER_HOST = "localhost";
    const std::string SERVER_PATH = "/";
    const std::string SERVER_ADDR = "127.0.0.1";
    const std::size_t SERVER_PORT = 80;
}

class IRequestHandler : public HTTPRequestHandler
{
public:
    IRequestHandler(const HTTPServer* server) : server_(server) {}

    void handleRequest(HTTPServerRequest& req, HTTPServerResponse& resp) override
    {
        HTTPRequest request(HTTPRequest::HTTP_GET, SERVER_PATH, HTTPMessage::HTTP_1_1);
        request.set("User-Agent", "poco/1.7.5");
        request.add("Accept","*/*");
        request.setHost(SERVER_HOST, SERVER_PORT);

        HTTPClientSession session(SERVER_ADDR, SERVER_PORT);
        session.setTimeout(TIMEOUT_MICROSECONDS);
        session.sendRequest(request);

//        std::cout << "--------------------------------" << std::endl;
//        request.write(std::cout);

        HTTPResponse response;
        std::istream& is = session.receiveResponse(response);

//        std::cout << "--------------------------------" << std::endl;
//        response.write(std::cout);

        resp.setStatus(response.getStatus());

        //resp.set("QueuedConnections", std::to_string(server_->queuedConnections()));

        auto it = response.begin();
        while(it != response.end()) {
            resp.set(it->first, it->second);
            ++it;
        }

        std::ostream& out = resp.send();
        StreamCopier::copyStream(is, out);
        out.flush();
    }

private:
    const HTTPServer* server_;
};

class IRequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest &) override
    {
        return new IRequestHandler(server_);
    }

    void setServer(HTTPServer* server) { server_ = server; }

private:
    HTTPServer* server_;
};

class IServerApplication : public ServerApplication
{
protected:
    int main(const std::vector<std::string>& args)
    {
        unsigned int hardware_concurrency = std::thread::hardware_concurrency();

        if (args.size() == 1) {
            hardware_concurrency = std::stoi(args[0]);
        }

        Poco::Net::HTTPServerParams::Ptr parameters = new Poco::Net::HTTPServerParams();
        parameters->setTimeout(TIMEOUT_MICROSECONDS);
        parameters->setMaxQueued(MAX_QUEUE);
        parameters->setMaxThreads(hardware_concurrency);
        parameters->setKeepAlive(true);

        const Poco::UInt16 port = 11111;
        const Poco::Net::ServerSocket socket(port);
        auto factory = new IRequestHandlerFactory();
        HTTPServer s(factory, socket, parameters);
        factory->setServer(&s);

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
