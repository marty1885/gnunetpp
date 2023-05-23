#include <drogon/drogon_test.h>
#include <drogon/HttpAppFramework.h>

#include <gnunetpp-crypto.hpp>
#include <gnunetpp-scheduler.hpp>
#include <gnunetpp-dht.hpp>
#include <gnunetpp-gns.hpp>
#include <gnunetpp-identity.hpp>
#include "inner/Infra.hpp"

#include <random>

using namespace drogon;
using namespace std::chrono_literals;

const GNUNET_CONFIGURATION_Handle* cfg = nullptr;
int status = 0;

static std::string randomString(size_t length)
{
    thread_local std::mt19937 rng{std::random_device{}()};
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<size_t> dist{0, sizeof(alphanum) - 2};

    std::string s;
    s.reserve(length);
    for (size_t i = 0; i < length; ++i)
    {
        s += alphanum[dist(rng)];
    }
    return s;
}

// MEGA HACK to get SANE coroutine environment
#define ENTER_MAIN_THREAD gnunetpp::async_run([=]() -> cppcoro::task<>{ co_await gnunetpp::scheduler::runOnMainThread();
#define EXIT_MAIN_THREAD });

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

    auto hash_str = crypto::to_string(hash);
    auto hash2 = crypto::hashCode(hash_str);
    CHECK(hash == hash2);
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

DROGON_TEST(ECDSA)
{
    auto sk = gnunetpp::crypto::anonymousKey();
    auto pk = gnunetpp::crypto::getPublicKey(sk);

    auto sig = gnunetpp::crypto::sign(sk, "hello world");
    REQUIRE(sig.has_value());
    CHECK(gnunetpp::crypto::verify(pk, "hello world", sig.value()));

    sig = gnunetpp::crypto::sign(sk, "hello world2");
    REQUIRE(sig.has_value());
    CHECK(gnunetpp::crypto::verify(pk, "hello world", sig.value()) == false);
}

DROGON_TEST(EdDSA)
{
    auto sk = gnunetpp::crypto::generateEdDSAPrivateKey();
    auto pk = gnunetpp::crypto::getPublicKey(sk);

    auto sig = gnunetpp::crypto::sign(sk, "hello world");
    REQUIRE(sig.has_value());
    CHECK(gnunetpp::crypto::verify(pk, "hello world", sig.value()));

    sig = gnunetpp::crypto::sign(sk, "hello world2");
    REQUIRE(sig.has_value());
    CHECK(gnunetpp::crypto::verify(pk, "hello world", sig.value()) == false);
}

DROGON_TEST(IdentityCrypto)
{
    auto sk = gnunetpp::crypto::myPeerPrivateKey(cfg);
    auto pk = gnunetpp::crypto::myPeerIdentity(cfg);

    auto sig = gnunetpp::crypto::sign(sk, "hello world");
    REQUIRE(sig.has_value());
    CHECK(gnunetpp::crypto::verify(pk.public_key, "hello world", sig.value()));
}

DROGON_TEST(TestRunsOnMainThread)
{
ENTER_MAIN_THREAD
    CO_REQUIRE(gnunetpp::inMainThread());
EXIT_MAIN_THREAD
}

DROGON_TEST(DHT)
{
ENTER_MAIN_THREAD

    auto dht = std::make_shared<gnunetpp::DHT>(cfg, 4);
    auto key = randomString(32);
    co_await dht->put(key, "world");
    // FIXME: Triggers Assertion failed at dht_api.c:1066
    // auto lookup = dht->get(key, 1s);
    // size_t count = 0;
    // for (auto it = co_await lookup.begin(); it != lookup.end(); co_await ++it) {
    //     CHECK(*it == "world");
    //     count++;
    // }
    // CHECK(count == 1);


EXIT_MAIN_THREAD
}

DROGON_TEST(Identity)
{
ENTER_MAIN_THREAD
    auto ego_name = randomString(32);
    auto identity = std::make_shared<gnunetpp::IdentityService>(cfg);
    co_await identity->createIdentity(ego_name, GNUNET_IDENTITY_TYPE_ECDSA);

    auto ego = co_await gnunetpp::getEgo(cfg, ego_name);
    CO_REQUIRE(ego.has_value());
    CHECK(ego->keyType() == GNUNET_IDENTITY_TYPE_ECDSA);

    // check signing with the identity works
    // FIXME: This API sucks and easy to confuse types
    auto sig = gnunetpp::crypto::sign(ego->privateKey()->ecdsa_key, "hello world");
    CHECK(sig.has_value());
    CHECK(gnunetpp::crypto::verify(ego->publicKey().ecdsa_key, "hello world", sig.value()));

    CHECK_NOTHROW(co_await identity->deleteIdentity(ego_name));
EXIT_MAIN_THREAD
}

DROGON_TEST(GNS)
{
ENTER_MAIN_THREAD
    auto gns = std::make_shared<gnunetpp::GNS>(cfg);
    auto result = co_await gns->lookup("gnunet.org", 10s, "ANY");
    CO_REQUIRE(result.size() != 0);
    // Won't check the actual result as it may change
EXIT_MAIN_THREAD
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