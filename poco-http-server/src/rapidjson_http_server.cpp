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

#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>

using namespace rapidjson;

class IRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
    void handleRequest(Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp) override
    {
        resp.set("Server", "pohttp");
        resp.setStatus(Poco::Net::HTTPResponse::HTTP_OK);

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);

        writer.StartObject();               // Between StartObject()/EndObject()
        writer.Key("Hello");                // output a key,
        writer.String("world");             // follow by a value.
        writer.Key("T");
        writer.Bool(true);
        writer.Key("F");
        writer.Bool(false);
        writer.Key("N");
        writer.Null();
        writer.Key("i");
        writer.Uint(123);
        writer.Key("PI");
        writer.Double(3.1416);
        writer.Key("Array");
        writer.StartArray();                // Between StartArray()/EndArray(),
        for (unsigned i = 0; i < 10; i++) {
            writer.Uint(i);                 // all values are elements of the array.
        }
        writer.EndArray();
        writer.EndObject();

        // {"hello":"world","t":true,"f":false,"n":null,"i":123,"pi":3.1416,"a":[0,1,2,3]}

        resp.setContentType("application/json");
        resp.setContentLength(buffer.GetLength());

        std::ostream& out = resp.send();
        out << buffer.GetString();
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
        if (args.empty()) {
            std::cerr << "Usage: server <port>\n";
            return Application::EXIT_OK;
        }

        unsigned int hardware_concurrency = std::thread::hardware_concurrency();

        Poco::Net::HTTPServerParams::Ptr parameters = new Poco::Net::HTTPServerParams();
        parameters->setTimeout(1000);
        parameters->setMaxQueued(1000);
        parameters->setMaxThreads(hardware_concurrency);

        const Poco::UInt16 port = std::stoi(args[0]);
        const Poco::Net::ServerSocket socket(port);
        Poco::Net::HTTPServer s(new IRequestHandlerFactory, socket, parameters);

        s.start();
        std::cout << "Server started: 127.0.0.1:" << port << std::endl;

        waitForTerminationRequest();  // wait for CTRL-C or kill

        std::cout << "Shutting down..." << std::endl;
        s.stop();

        return Application::EXIT_OK;
    }
};

int main(int argc, char* argv[])
{
    IServerApplication app;
    return app.run(argc, argv);
}
