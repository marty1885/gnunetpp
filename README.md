# GNUNet++ - A C++ wrapper for GNUNet

## Why GNUNet++?

IMO GNUNet is a cool project which shows what decentralized systems could look like. However the [time to first hello world][ttfhw] is way too long, API is not well documented and everything is written GNU style C. Which are all factors against it's adoption. GNUNet++ wraps all the C-ness into a (more or less) modern C++ style API. Provides proper lifetime management and an easier learning path. Hopefully leading to a better development experience.

[ttfhw]: https://www.moesif.com/blog/technical/api-product-management/What-is-TTFHW/ 

## Show me some code!

The DHT class provides easy access to GNUnet's DHT. It's API is designed to look like [OpenDHT][opendht]'s for more familiarity to potential users. Note that need GNUNet running on your system before running the samples.

```cpp
// Max 32 simultaneous operations
auto dht = std::make_shared<DHT>(cfg, 32);

...
dht->put("Answer", "42");
dht->get("some_super_secret", [](const std::string_view data) {
   std::cout << "super secret info: " << data << std::endl;
   return true; // keep on searching after the current one.
   // return false to cancel futher lookups.
}, std::chrono::seconds(30)); // Max searching for 30 seconds

```

[opendht]: https://github.com/savoirfairelinux/opendht