#include <drogon/drogon_test.h>
#include <drogon/HttpAppFramework.h>

#include <gnunetpp-crypto.hpp>
#include <gnunetpp-scheduler.hpp>
#include "inner/Infra.hpp"

using namespace drogon;

const GNUNET_CONFIGURATION_Handle* cfg = nullptr;
int status = 0;

template <typename T>
concept SupportsAllCompares = requires(T t) {
    { t == t } -> std::same_as<bool>;
    { t != t } -> std::same_as<bool>;
    { t < t } -> std::same_as<bool>;
    { t > t } -> std::same_as<bool>;
    { t <= t } -> std::same_as<bool>;
    { t >= t } -> std::same_as<bool>;
    { t <=> t } -> std::same_as<std::strong_ordering>;
};

template <typename T>
concept SupportsHash = requires(T t) { std::hash<T>{}(t); };

DROGON_TEST(CryptoTest)
{
    using namespace gnunetpp;

    auto hash = crypto::hash("hello world");
    CHECK(hash == crypto::hash("hello world"));
    CHECK(hash != crypto::hash("hello world2"));
    STATIC_REQUIRE(SupportsAllCompares<GNUNET_HashCode>);
    STATIC_REQUIRE(SupportsHash<GNUNET_HashCode>);

    hash = crypto::hmac("hello world", "key");
    CHECK(hash == crypto::hmac("hello world", "key"));
    CHECK(hash != crypto::hmac("hello world2", "key"));
    CHECK(hash != crypto::hmac("hello world", "key2"));

    STATIC_REQUIRE(SupportsAllCompares<GNUNET_PeerIdentity>);
    STATIC_REQUIRE(SupportsHash<GNUNET_PeerIdentity>);
}

DROGON_TEST(Sechduler)
{
    using namespace gnunetpp;
    using namespace std::chrono_literals;
    scheduler::runLater(10ms, [TEST_CTX]() {
        SUCCESS();
    });
    scheduler::run([TEST_CTX]() {
        SUCCESS();
    });
    scheduler::queue([TEST_CTX]() {
        SUCCESS();
    });

    gnunetpp::async_run([TEST_CTX]() -> cppcoro::task<void> {
        co_await scheduler::sleep(10ms);
        SUCCESS();
    });

    // call from another thread
    app().getLoop()->queueInLoop([TEST_CTX]() {
        scheduler::run([TEST_CTX]() {
            SUCCESS();
        });
    });
}

int main(int argc, char** argv)
{
    gnunetpp::run([=](const GNUNET_CONFIGURATION_Handle* cfg_) {
        cfg = cfg_;
        std::thread thr([=]{
            std::promise<void> p1;
            std::future<void> f1 = p1.get_future();

            // Start the main loop on another thread
            std::jthread thr([&]() {
                // Queues the promise to be fulfilled after starting the loop
                app().getLoop()->queueInLoop([&p1]() { p1.set_value(); });
                app().run();
            });

            // The future is only satisfied after the event loop started
            f1.get();
            status = test::run(argc, argv);

            // Ask the event loop to shutdown and wait
            app().getLoop()->queueInLoop([]() {
                app().quit();
                gnunetpp::shutdown();
            });
        });
        thr.detach();
    });

    return status;
}