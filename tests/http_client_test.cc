#include "gtest/gtest.h"

#include <pistache/http.h>
#include <pistache/client.h>
#include <pistache/endpoint.h>

#include <chrono>
#include "test_utils.h"

using namespace Pistache;

struct HelloHandler : public Http::Handler {
    HTTP_PROTOTYPE(HelloHandler)

    void onRequest(const Http::Request& request, Http::ResponseWriter writer)
    {
        UNUSED(request)
        writer.send(Http::Code::Ok, "Hello, World!");
    }
};

TEST(request_builder, multiple_send_test) {
    uint16_t port_nb = Pistache::Test::find_port();
    if (port_nb == 0) {
        FAIL() << "Could not find a free port. Abord test.\n";
    }
    std::stringstream ss;
    ss << "localhost:" << port_nb;
    const std::string address = ss.str();

    Http::Endpoint server(address);
    auto flags = Tcp::Options::ReuseAddr;
    auto server_opts = Http::Endpoint::options().flags(flags).threads(1);
    server.init(server_opts);
    server.setHandler(Http::make_handler<HelloHandler>());
    server.serveThreaded();

    Http::Client client;
    auto client_opts = Http::Client::options().threads(1).maxConnectionsPerHost(1);
    client.init(client_opts);

    std::vector<Async::Promise<Http::Response>> responses;
    const int RESPONSE_SIZE = 3;
    int response_counter = 0;

    auto rb = client.get(address);
    for (int i = 0; i < RESPONSE_SIZE; ++i) {
        auto response = rb.send();
        response.then([&](Http::Response rsp) {
            if (rsp.code() == Http::Code::Ok)
                ++response_counter;
        }, Async::IgnoreException);
        responses.push_back(std::move(response));
    }

    auto sync = Async::whenAll(responses.begin(), responses.end());
    Async::Barrier<std::vector<Http::Response>> barrier(sync);

    barrier.wait_for(std::chrono::seconds(5));

    server.shutdown();
    client.shutdown();

    ASSERT_TRUE(response_counter == RESPONSE_SIZE);
}