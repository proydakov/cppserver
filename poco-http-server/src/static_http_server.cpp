#include <thread>
#include <string>
#include <chrono>
#include <iostream>

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Util/ServerApplication.h>

#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>

using namespace Poco::Net;
using namespace Poco::Util;

using namespace rapidjson;

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
<a href="https://github.com/proydakov/cpp-server/tree/master/poco-http-server">poco-http-server</a>.<br/>

<p><em>Thank you for using POCO.</em></p>
</body>
</html>
)";

class RequestHandler : public HTTPRequestHandler
{
public:
    RequestHandler(HTTPServer const& server) :
        server_(server)
    {
    }

    void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) override
    {
        auto const& uri = req.getURI();

        if("/" == uri) {
            handleRequestStatic(req, resp);
        }
        else if("/status" == uri) {
            handleRequestStatus(req, resp);
        }
        else {
            resp.setStatus(HTTPResponse::HTTP_NOT_FOUND);
            std::ostream& out = resp.send();
            out.flush();
        }
    }

private:
    void handleRequestStatic(HTTPServerRequest &req, HTTPServerResponse &resp)
    {
        resp.setStatus(HTTPResponse::HTTP_OK);
        resp.setContentType("text/html");
        resp.setContentLength(PAGE.size());

        std::ostream& out = resp.send();
        out << PAGE;

        out.flush();
    }

    void handleRequestStatus(HTTPServerRequest &req, HTTPServerResponse &resp)
    {
        resp.setStatus(HTTPResponse::HTTP_OK);
        resp.setContentType("application/json");

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);

        writer.StartObject();

        writer.Key("currentConnections");
        writer.Uint(server_.currentConnections());

        writer.Key("queuedConnections");
        writer.Uint(server_.queuedConnections());

        writer.EndObject();

        resp.setContentLength(buffer.GetLength());

        std::ostream& out = resp.send();
        out << buffer.GetString();
        out.flush();
    }

private:
    HTTPServer const& server_;
};

class RequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest &) override
    {
        return new RequestHandler(*server_);
    }

    void setServer(HTTPServer const* server)
    {
        server_ = server;
    }

private:
    HTTPServer const* server_ = nullptr;
};

class IServerApplication : public ServerApplication
{
protected:
    int main(const std::vector<std::string>& args)
    {
        unsigned int hardware_concurrency = std::thread::hardware_concurrency();

        HTTPServerParams::Ptr parameters = new HTTPServerParams();
        parameters->setTimeout(10000);
        parameters->setMaxQueued(10000);
        parameters->setMaxThreads(hardware_concurrency);

        const Poco::UInt16 port = 11111;
        ServerSocket socket(port);
        socket.setReuseAddress(true);
        socket.setReusePort(true);

        auto factory = new RequestHandlerFactory();
        HTTPServer s(factory, socket, parameters);
        factory->setServer(&s);

        s.start();
        std::cout << "server started: 127.0.0.1:" << port << std::endl;

        waitForTerminationRequest();  // wait for CTRL-C or kill

        std::cout << "shutting down... " << std::endl;
        s.stop();

        return Application::EXIT_OK;
    }
};

}

int main(int argc, char** argv)
{
    IServerApplication app;

    return app.run(argc, argv);
}
