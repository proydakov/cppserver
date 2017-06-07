#include <thread>
#include <iostream>

#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>

class IRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    void handleRequest(Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp) override
    {
        resp.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
        resp.setContentType("application/json");

        std::ostream& out = resp.send();

        out << R"({"robot":"webot"})";
        out.flush();
    }
};

class IRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest&) override
    {
        return new IRequestHandler;
    }
};

class IServerApplication : public Poco::Util::ServerApplication
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
        Poco::Net::HTTPServer s(new IRequestHandlerFactory, socket, parameters);

        s.start();
        std::cout << "server started: 127.0.0.1:" << port << std::endl;

        waitForTerminationRequest();  // wait for CTRL-C or kill

        std::cout << "shutting down..." << std::endl;
        s.stop();

        return Application::EXIT_OK;
    }
};

int main(int argc, char* argv[])
{
    IServerApplication app;
    return app.run(argc, argv);
}
