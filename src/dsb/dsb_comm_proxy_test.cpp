#include "gtest/gtest.h"
#include "dsb/config.h"
#include "dsb/comm/proxy.hpp"

using namespace dsb::comm;


TEST(dsb_proxy, unidirectional)
{
    auto ctx = std::make_shared<zmq::context_t>();
    auto proxy = SpawnProxy(ctx,
        ZMQ_PULL, "inproc://dsb_proxy_test_frontend",
        ZMQ_PUSH, "inproc://dsb_proxy_test_backend");

    auto fe = zmq::socket_t(*ctx, ZMQ_PUSH);
    fe.connect("inproc://dsb_proxy_test_frontend");
    auto be = zmq::socket_t(*ctx, ZMQ_PULL);
    be.connect("inproc://dsb_proxy_test_backend");

    for (char ctx = 0; ctx < 10; ++ctx) {
        fe.send(&ctx, sizeof(ctx));
    }
    for (char ctx = 0; ctx < 10; ++ctx) {
        char buf = -1;
        EXPECT_EQ(sizeof(buf), be.recv(&buf, sizeof(buf)));
        EXPECT_EQ(ctx, buf);
    }
    proxy.Stop();
    EXPECT_TRUE(proxy.Thread__().try_join_for(boost::chrono::milliseconds(10)));
}


TEST(dsb_proxy, bidirectional_multiclient)
{
    auto ctx = std::make_shared<zmq::context_t>();
    auto proxy = SpawnProxy(ctx,
        ZMQ_ROUTER, "inproc://dsb_proxy_test_frontend",
        ZMQ_DEALER, "inproc://dsb_proxy_test_backend");

    const int CLIENT_COUNT = 10;
    std::vector<zmq::socket_t> clients;
    for (int k = 0; k < CLIENT_COUNT; ++k) {
#if DSB_USE_MSVC_EMPLACE_WORKAROUND
        clients.emplace_back(zmq::socket_t(*ctx, ZMQ_REQ));
#else
        clients.emplace_back(*ctx, ZMQ_REQ);
#endif
        clients.back().connect("inproc://dsb_proxy_test_frontend");
    }
    auto server = zmq::socket_t(*ctx, ZMQ_REP);
    server.connect("inproc://dsb_proxy_test_backend");

    for (int i = 0; i < 100; ++i) {
        for (int k = 0; k < CLIENT_COUNT; ++k) {
            const auto req = i*k;
            clients[k].send(&req, sizeof(req));
        }
        for (int k = 0; k < CLIENT_COUNT; ++k) {
            int req = -1;
            server.recv(&req, sizeof(req));
            req += 123;
            server.send(&req, sizeof(req));
        }
        for (int k = 0; k < CLIENT_COUNT; ++k) {
            int rep = -1;
            clients[k].recv(&rep, sizeof(rep));
            EXPECT_EQ(i*k+123, rep);
        }
    }
    proxy.Stop();
    EXPECT_TRUE(proxy.Thread__().try_join_for(boost::chrono::milliseconds(10)));
}


TEST(dsb_proxy, silence_timeout)
{
    using namespace boost::chrono;
    using namespace boost::this_thread;

    auto ctx = std::make_shared<zmq::context_t>();
    auto proxy = SpawnProxy(ctx,
        ZMQ_PULL, "inproc://dsb_proxy_test_frontend",
        ZMQ_PUSH, "inproc://dsb_proxy_test_backend",
        milliseconds(200));
    auto push = zmq::socket_t(*ctx, ZMQ_PUSH);
    push.connect("inproc://dsb_proxy_test_frontend");
    auto pull = zmq::socket_t(*ctx, ZMQ_PULL);
    pull.connect("inproc://dsb_proxy_test_backend");

    sleep_for(milliseconds(100));
    push.send("", 0);
    sleep_for(milliseconds(150));
    ASSERT_FALSE(proxy.Thread__().try_join_for(milliseconds(0)));
    ASSERT_TRUE(proxy.Thread__().try_join_for(milliseconds(70)));
}
