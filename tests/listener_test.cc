#include "gtest/gtest.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pistache/listener.h>
#include <pistache/http.h>
#include "test_utils.h"

// Just there for show.
class DummyHandler : public Pistache::Http::Handler {
public:

HTTP_PROTOTYPE(DummyHandler)

    void onRequest(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter response) override {
        UNUSED(request);
        response.send(Pistache::Http::Code::Ok, "I am a dummy handler\n");
    }
};


TEST(listener_test, listener_bind_port_free) {
    // In theory, it is possible that some application grab this port before we bind it again...
    uint16_t port_nb = Pistache::Test::find_port();
    if (port_nb == 0) {
        FAIL() << "Could not find a free port. Abord test.\n";
    }

    Pistache::Port port(port_nb);
    Pistache::Address address(Pistache::Ipv4::any(), port);

    Pistache::Tcp::Listener listener;
    Pistache::Flags<Pistache::Tcp::Options> options;
    listener.init(1, options);
    listener.setHandler(Pistache::Http::make_handler<DummyHandler>());
    listener.bind(address);
    ASSERT_TRUE(true);
}


TEST(listener_test, listener_bind_port_not_free_throw_runtime) {

    Pistache::Test::SocketWrapper s = Pistache::Test::bind_free_port();
    uint16_t port_nb = s.port();

    if (port_nb == 0) {
        FAIL() << "Could not find a free port. Abord test.\n";
    }

    Pistache::Port port(port_nb);
    Pistache::Address address(Pistache::Ipv4::any(), port);

    Pistache::Tcp::Listener listener;
    Pistache::Flags<Pistache::Tcp::Options> options;
    listener.init(1, options);
    listener.setHandler(Pistache::Http::make_handler<DummyHandler>());

    try {
        listener.bind(address);
        FAIL() << "Expected std::runtime_error while binding, got nothing";
    } catch (std::runtime_error const & err) {
        std::cout << err.what() << std::endl;
        ASSERT_STREQ("Address already in use", err.what());
    } catch ( ... ) {
        FAIL() << "Expected std::runtime_error";
    }
}
