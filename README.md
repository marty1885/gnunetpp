# GNUNet++ - A C++ wrapper for GNUNet

## Why GNUNet++?

IMO GNUNet is a cool project which shows what decentralized systems could look like. However the [time to first hello world][ttfhw] is way too long, API is not well documented and everything is written GNU style C. Which are all factors against it's adoption. GNUNet++ wraps all the C-ness into a (more or less) modern C++ style API. Provides proper lifetime management and an easier learning path. Hopefully leading to a better development experience.

**IMPORTAN**: This is not a part of the GNUNet project. The [author](https://github.com/marty1885) created it because he wants to use GNUNet in C++.

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

The cryptographic library is also easily accessable

```cpp
auto hash = crypto::hash("foobar");
auto hmac = crypto::hmac("hmac_key", "zap");
std::cout << crypto::to_string(hmac) << "\n";
```

[opendht]: https://github.com/savoirfairelinux/opendht

## Requirments

First you need the dependencies

* C++17 capable compiler
* A installation on GNUNet

## Roadmap

This project aims to create a easy to use wapper for the commonly used part of GNUNet.

- DHT
  - [x] Basic operations (put/get)
  - [ ] Monitor
- File Sharing
  - [ ] Download
  - [ ] Publish
  - [ ] Search
- Crypto
  - [x] Hash
  - [x] HMAC
  - [ ] ECDSA/EDDSA/ECDHE
  - [ ] to_string
- CADET
  - [ ] Server
  - [ ] Client
- GNS
  - [ ] Resolve
  - [ ] Register/modify record
  - [ ] Ego
- Scheduler
  - [x] Delayed run
  - [x] Run on exit
  - [x] Run immidately
